//----------------------------------------------------------------------------
//! @file   DeskHud.cpp
//! @brief  対局画面の机の上に常に出しておく紙片の実装
//----------------------------------------------------------------------------
#include <ChronoSort/UI/DeskHud.hpp>

#include <ChronoSort/Board/BoardLayout.hpp>
#include <ChronoSort/ECS/Component/CountUpTextComponent.hpp>
#include <ChronoSort/Game/TuningSet.hpp>
#include <ChronoSort/Match/MatchEvents.hpp>
#include <ChronoSort/UI/TweenTags.hpp>
#include <ChronoSort/Util/AssetPaths.hpp>
#include <ChronoSort/Util/TextUtil.hpp>

#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/WorldAnchorComponent.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UITweenUtility.hpp>
#include <Tsukino/Core/ECS/Event/EventBus.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>

#include <algorithm>
#include <filesystem>
#include <string>

namespace ChronoSort::UI {

    namespace {
        namespace BuiltIn = Tsukino::BuiltIn::ECS;
        using Tsukino::Core::Math::Easing::EaseType;

        constexpr float kDegToRad = 3.14159265f / 180.0f;

        // 文字の描画順（大きいほど手前）。紙片の描画順は Data/UI/GameHud.json に書き、文字はその上に重ねる
        constexpr int kLayerDesk     = 10;
        constexpr int kLayerAnchored = 40;

        constexpr size_t kDeckWarnCount = 5;    // 山札がこの枚数以下で札を弾ませる

        //! 数字を全角にせず、そのままワイド文字列にします。
        std::wstring Number(long long value) {
            return std::to_wstring(value);
        }

        //! 一瞬弾んでは休む、を繰り返させます（警告や催促に使う）。
        void PlayRepeatingPunch(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, BuiltIn::UITweenProperty property, float from, float to,
                                float duration, float rest, std::uint32_t tag) {
            BuiltIn::UITween punch;
            punch.property    = property;
            punch.from        = from;
            punch.to          = to;
            punch.duration    = duration;
            punch.repeatDelay = rest;
            punch.ease        = EaseType::Punch;
            punch.loop        = BuiltIn::UITweenLoop::Loop;
            punch.tag         = tag;
            BuiltIn::UITweenUtility::Play(registry, entity, punch);
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! 作り先のシーン・アセット管理・表示する対局を受け取り、出来事の購読を始めます。
    //----------------------------------------------------------------------------
    DeskHud::DeskHud(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager, const Rules::Match& match)
        : m_scene(scene)
        , m_ui(scene, assetManager)
        , m_match(match)
        , m_layout(Util::AssetPaths::UILayout("GameHud")) {
        Tsukino::ECS::EventBus& events = scene.GetEventBus();
        auto                    banner = [](const std::string& text) { return Util::Utf8ToWide(text); };

        m_connections.push_back(events.Subscribe<Match::MatchStateChangedEvent>([this](const Match::MatchStateChangedEvent&) { Refresh(); }));
        m_connections.push_back(events.Subscribe<Match::TurnStartedEvent>([this, banner](const Match::TurnStartedEvent& e) {
            SetTurn(e.player);
            if(e.human && m_match.Config().mode == Rules::MatchMode::Versus) {
                ShowBanner(banner(Game::Tuning().Feedback().bannerYourTurn), false);
            }
        }));
        m_connections.push_back(events.Subscribe<Match::TiebreakStartedEvent>(
            [this, banner](const Match::TiebreakStartedEvent&) { ShowBanner(banner(Game::Tuning().Feedback().bannerTiebreak), true); }));
        m_connections.push_back(events.Subscribe<Match::PlayJudgedEvent>([this](const Match::PlayJudgedEvent& e) { OnPlayResolved(e.player, e.result); }));
        m_connections.push_back(events.Subscribe<Match::MatchEndedEvent>([this, banner](const Match::MatchEndedEvent& e) {
            const Match::FeedbackTuning& t = Game::Tuning().Feedback();
            ShowBanner(banner(e.won ? t.bannerWon : (e.outcome == Rules::MatchOutcome::SoloFailed ? t.bannerInkOut : t.bannerLost)), true);
        }));
        m_connections.push_back(events.Subscribe<Match::IdleNudgeEvent>([this](const Match::IdleNudgeEvent& e) { SetNudge(e.active); }));
    }

    //----------------------------------------------------------------------------
    //! 紙片を作ります。
    //----------------------------------------------------------------------------
    void DeskHud::Build() {
        const bool solo = m_match.Config().mode == Rules::MatchMode::Solo;

        // 画面の四隅を暗くして、ランプに照らされた机に見せる
        {
            const auto vignette = m_ui.Paper(m_layout.Spec("vignette"));
            m_scene.GetRegistry().GetComponent<BuiltIn::SpriteComponent>(vignette).raycastTarget = false;
        }

        // 封蝋（ソロは得点を数え上げる、対戦は巡目）と、その札
        {
            const auto tag = m_ui.Paper(m_layout.Spec("sealLabel"));
            m_ui.Text(solo ? L"得点" : L"巡目", hlslpp::float2(8.0f, 2.0f), 0.62f, kInkColor, kLayerDesk + 1, tag);
            m_ui.SlideIn(tag, hlslpp::float2(-260.0f, 0.0f), 0.2f);

            m_seal     = m_ui.Paper(m_layout.Spec("seal"));
            m_sealText = m_ui.Text(L"0", hlslpp::float2(0.0f, -2.0f), 1.15f, kParchmentColor, kLayerDesk + 3, m_seal);
            if(solo) {
                m_scene.GetRegistry().AddComponent<ECS::CountUpTextComponent>(m_sealText);
            }
            m_ui.SlideIn(m_seal, hlslpp::float2(-260.0f, -40.0f), 0.1f, 0.55f);
        }

        // 出題の範囲を絞っているときだけ、その名前の札を添える（通史なら今までどおり何も出さない）
        if(m_match.Config().range != Rules::CardRange::All) {
            const auto tag = m_ui.Paper(m_layout.Spec("rangeTag"));
            m_ui.Text(Util::Utf8ToWide(Rules::RangeName(m_match.Config().range)), hlslpp::float2(4.0f, 1.0f), 0.5f, kSealRedColor, kLayerDesk + 1, tag);
            m_ui.SlideIn(tag, hlslpp::float2(-200.0f, -60.0f), 0.3f);
        }

        // ライフのインク壺（ソロ）
        if(solo) {
            for(int i = 0; i < m_match.Config().rules.soloLives; ++i) {
                m_inks.push_back(m_ui.Paper(m_layout.Spec("ink" + std::to_string(i))));
                m_ui.SlideIn(m_inks.back(), hlslpp::float2(-200.0f, 0.0f), 0.3f + 0.08f * static_cast<float>(i));
            }
            m_shownLives = m_match.Config().rules.soloLives;
        }

        // 山札の札（山札の 3D の近くにピンで留める）
        {
            m_deckTag  = m_ui.Paper(m_layout.Spec("deckTag"));
            m_deckText = m_ui.Text(L"山札 0", hlslpp::float2(10.0f, 2.0f), 0.62f, kInkColor, kLayerDesk + 1, m_deckTag);

            PaperSpec pin = m_layout.Spec("deckPin");
            pin.parent    = m_deckTag;
            m_ui.Paper(pin);
            m_ui.SlideIn(m_deckTag, hlslpp::float2(300.0f, 0.0f), 0.4f);
        }

        // 操作説明（最初の1枚を置いたら片付ける）
        {
            m_hint = m_ui.Paper(m_layout.Spec("hint"));
            m_ui.Text(L"カードを掴んで、年代の合う隙間へ", hlslpp::float2(0.0f, -14.0f), 0.62f, kInkColor, kLayerDesk + 1, m_hint);
            m_ui.Text(L"クリックで選んでから置くこともできます", hlslpp::float2(0.0f, 20.0f), 0.46f, kFadedInkColor, kLayerDesk + 1, m_hint);

            PaperSpec tape = m_layout.Spec("hintTape");
            tape.parent    = m_hint;
            m_ui.Paper(tape);
            m_ui.SlideIn(m_hint, hlslpp::float2(0.0f, 200.0f), 1.2f);
        }

        // 各プレイヤーの名札。人間は左下、CPU は卓の奥の席の上に浮かべる
        const auto& players = m_match.Players();
        for(size_t i = 0; i < players.size(); ++i) {
            Tsukino::ECS::Entity tag;
            if(!players[i].isCpu) {
                tag = m_ui.Paper(m_layout.Spec("playerTag"));
                m_ui.Text(Util::Utf8ToWide(players[i].name), hlslpp::float2(4.0f, 0.0f), 0.6f, kInkColor, kLayerDesk + 1, tag);
                m_handCounts.push_back(entt::null);
            } else {
                // 額の傾きは人ごとにばらす
                PaperSpec frame    = m_layout.Spec("cpuFrame");
                frame.angleDegrees += m_ui.Jitter() * 6.0f;
                tag                = m_ui.Paper(frame);

                // 卓の奥の席の上に貼り付ける（カメラが動いても席についてくる）
                auto& anchor                 = m_scene.GetRegistry().AddComponent<BuiltIn::WorldAnchorComponent>(tag);
                anchor.useFixedWorldPosition = true;
                anchor.fixedWorldPosition    = Board::SeatPosition(i - 1, players.size() - 1);
                // 席の手前は起こした場の列と重なるので、席の内側（卓の中央寄り）の横に並べる
                const float side             = (float(anchor.fixedWorldPosition.x) > 0.0f) ? -1.0f : 1.0f;
                anchor.worldOffset           = hlslpp::float3(side * 175.0f, 0.0f, 0.0f);
                anchor.screenOffset          = hlslpp::float2(0.0f, -8.0f);

                // 歴史家の肖像（取得できていなければ影絵）
                const std::string& portrait = players[i].portrait;
                const bool         hasFace  = !portrait.empty() && std::filesystem::exists(Util::AssetPaths::UITexture(portrait));

                PaperSpec face = m_layout.Spec("cpuFace");
                face.texture   = hasFace ? portrait : "silhouette.png";
                face.parent    = tag;
                m_ui.Paper(face);
                m_ui.Text(Util::Utf8ToWide(players[i].name), hlslpp::float2(0.0f, 41.0f), 0.42f, kInkColor, kLayerAnchored + 2, tag);
                // 手札の枚数は、席のカードと反対の側に出す
                m_handCounts.push_back(m_ui.Text(L"", hlslpp::float2(side * 100.0f, 0.0f), 0.46f, kParchmentColor, kLayerAnchored + 2, tag, true));
            }
            m_playerTags.push_back(tag);
        }

        // 手番のリボン（名札にぶら下げる）
        m_ribbon = m_ui.Paper(m_layout.Spec("ribbon"));
    }

    //----------------------------------------------------------------------------
    //! 規則の値を表示に合わせます。
    //----------------------------------------------------------------------------
    void DeskHud::Refresh() {
        const bool              solo     = m_match.Config().mode == Rules::MatchMode::Solo;
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();

        // 封蝋：得点は数え上げる（CountUpTextSystem）。対戦は巡目
        if(solo) {
            registry.GetComponent<ECS::CountUpTextComponent>(m_sealText).target = static_cast<float>(m_match.Score());
        } else if(m_match.Round() != m_shownRound) {
            m_shownRound = m_match.Round();
            m_ui.SetText(m_sealText, Number(m_shownRound));
            m_ui.Punch(m_seal, 0.2f);
        }

        if(solo && m_match.Lives() != m_shownLives) {
            RefreshLives();
        }

        if(m_match.DeckRemaining() != m_shownDeck) {
            m_shownDeck = m_match.DeckRemaining();
            m_ui.SetText(m_deckText, L"山札 " + Number(static_cast<long long>(m_shownDeck)));
        }

        // 残りわずかの警告：最後のインク壺は震え、山札の札は弾む（変わった瞬間だけ付け外しする）
        const bool lifeWarning = solo && m_match.Lives() == 1 && !m_inks.empty();
        if(lifeWarning != m_lifeWarning) {
            m_lifeWarning = lifeWarning;
            if(lifeWarning) {
                PlayRepeatingPunch(registry, m_inks.front(), BuiltIn::UITweenProperty::Rotation, 0.0f, 6.0f * kDegToRad, 0.4f, 1.6f, Tag(TweenTag::LifeWarning));
            } else {
                for(const auto ink : m_inks) {
                    BuiltIn::UITweenUtility::Stop(registry, ink, Tag(TweenTag::LifeWarning));
                }
            }
        }
        const bool deckWarning = m_shownDeck > 0 && m_shownDeck <= kDeckWarnCount;
        if(deckWarning != m_deckWarning) {
            m_deckWarning = deckWarning;
            if(deckWarning) {
                PlayRepeatingPunch(registry, m_deckTag, BuiltIn::UITweenProperty::Scale, 1.0f, 1.12f, 0.4f, 1.2f, Tag(TweenTag::DeckWarning));
            } else {
                BuiltIn::UITweenUtility::Stop(registry, m_deckTag, Tag(TweenTag::DeckWarning));
            }
        }

        // CPU の手札枚数（変わったときだけ書き換える。脱落は -1 として扱う）
        const auto& players = m_match.Players();
        m_shownHandCounts.resize(m_handCounts.size(), -2);
        for(size_t i = 0; i < players.size() && i < m_handCounts.size(); ++i) {
            const int count = players[i].eliminated ? -1 : static_cast<int>(players[i].hand.size());
            if(m_handCounts[i] == entt::null || m_shownHandCounts[i] == count) {
                continue;
            }
            m_shownHandCounts[i] = count;
            m_ui.SetText(m_handCounts[i], count < 0 ? L"脱落" : (L"手札 " + Number(count)));
        }
    }

    //----------------------------------------------------------------------------
    //! ライフのインク壺を更新します。
    //----------------------------------------------------------------------------
    void DeskHud::RefreshLives() {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        const int               lives    = std::max(0, m_match.Lives());
        for(int i = lives; i < m_shownLives && i < static_cast<int>(m_inks.size()); ++i) {
            // 右のインク壺から倒れて空になる
            const Tsukino::ECS::Entity ink = m_inks[static_cast<size_t>(i)];
            m_ui.SetTexture(ink, "ink_empty.png");
            m_ui.StopIdle(ink);    // 倒れた壺は横たわったまま動かさない
            BuiltIn::UITween tip;
            tip.property  = BuiltIn::UITweenProperty::Rotation;
            tip.from      = 0.0f;
            tip.to        = 78.0f * kDegToRad;
            tip.duration  = 0.5f;
            tip.ease      = EaseType::OutBounce;
            tip.holdAtEnd = true;
            tip.tag       = Tag(TweenTag::InkTip);
            BuiltIn::UITweenUtility::Play(registry, ink, tip);

            BuiltIn::UITween slide;
            slide.property  = BuiltIn::UITweenProperty::OffsetY;
            slide.from      = 0.0f;
            slide.to        = 18.0f;
            slide.duration  = 0.5f;
            slide.ease      = EaseType::OutBounce;
            slide.holdAtEnd = true;
            slide.tag       = Tag(TweenTag::InkSlide);
            BuiltIn::UITweenUtility::Play(registry, ink, slide);
        }
        m_shownLives = lives;
    }

    //----------------------------------------------------------------------------
    //! 手番のプレイヤーを示します。
    //----------------------------------------------------------------------------
    void DeskHud::SetTurn(size_t player) {
        if(player >= m_playerTags.size() || m_ribbon == entt::null) {
            return;
        }
        Tsukino::ECS::Registry& registry  = m_scene.GetRegistry();
        auto&                   transform = registry.GetComponent<BuiltIn::TransformComponent>(m_ribbon);
        transform.parent                  = m_playerTags[player];
        transform.position                = m_match.Players()[player].isCpu ? hlslpp::float3(40.0f, -34.0f, 0.0f) : hlslpp::float3(82.0f, -46.0f, 0.0f);
        transform.dirty                   = true;
        m_ui.SlideIn(m_ribbon, hlslpp::float2(0.0f, -60.0f), 0.0f, 0.4f);
        m_ui.Punch(m_playerTags[player], 0.12f);

        // 手番の名札だけを大きめに呼吸させる
        if(m_turnPlayer != player) {
            if(m_turnPlayer < m_playerTags.size()) {
                BuiltIn::UITweenUtility::Stop(registry, m_playerTags[m_turnPlayer], Tag(TweenTag::TurnBreathe));
            }
            BuiltIn::UITween breathe;
            breathe.property = BuiltIn::UITweenProperty::Scale;
            breathe.from     = 1.0f;
            breathe.to       = 1.05f;
            breathe.duration = 0.9f;
            breathe.ease     = EaseType::InOutSine;
            breathe.loop     = BuiltIn::UITweenLoop::PingPong;
            breathe.tag      = Tag(TweenTag::TurnBreathe);
            BuiltIn::UITweenUtility::Play(registry, m_playerTags[player], breathe);
            m_turnPlayer = player;
        }
    }

    //----------------------------------------------------------------------------
    //! 放置されているあいだ、次の行動を促します。
    //----------------------------------------------------------------------------
    void DeskHud::SetNudge(bool active) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        if(m_nudged != entt::null) {
            BuiltIn::UITweenUtility::Stop(registry, m_nudged, Tag(TweenTag::IdleNudge));
            m_nudged = entt::null;
        }
        if(!active) {
            return;
        }
        // 操作説明が残っていればそれを、片付いていれば手番のリボンを震わせる
        const Tsukino::ECS::Entity target = (m_hint != entt::null) ? m_hint : m_ribbon;
        if(target == entt::null) {
            return;
        }
        PlayRepeatingPunch(registry, target, BuiltIn::UITweenProperty::Rotation, 0.0f, 5.0f * kDegToRad, 0.5f, 2.5f, Tag(TweenTag::IdleNudge));
        m_nudged = target;
    }

    //----------------------------------------------------------------------------
    //! 通知の帯を滑り込ませ、少し置いてから抜きます。
    //----------------------------------------------------------------------------
    void DeskHud::ShowBanner(const std::wstring& text, bool big) {
        // 帯は毎回少しずつ位置と傾きをずらす
        PaperSpec spec    = m_layout.Spec(big ? "bannerBig" : "banner");
        spec.center       = spec.center + hlslpp::float2(m_ui.Jitter() * 30.0f, 0.0f);
        spec.angleDegrees = m_ui.Jitter() * 4.0f;
        const auto banner = m_ui.Paper(spec);
        m_ui.Text(text, hlslpp::float2(0.0f, 0.0f), big ? 1.7f : 1.15f, kInkColor, spec.sortOrder + 1, banner);

        const float stay = big ? 2.0f : 1.1f;
        m_ui.SlideIn(banner, hlslpp::float2(-kReferenceWidth * 0.7f, 30.0f), 0.0f, 0.5f);
        m_ui.SlideOut(banner, hlslpp::float2(kReferenceWidth * 0.7f, -30.0f), 0.5f + stay, 0.4f);
        m_ui.DestroyAfter(banner, 1.0f + stay);
    }

    //----------------------------------------------------------------------------
    //! 1枚の判定が終わったときの表示を行います。
    //----------------------------------------------------------------------------
    void DeskHud::OnPlayResolved(size_t player, const Rules::PlayResult& result) {
        // 最初に人間が置いたら、操作説明は役目を終える
        if(player == 0 && m_hint != entt::null) {
            if(m_nudged == m_hint) {
                m_nudged = entt::null;
            }
            m_ui.SlideOut(m_hint, hlslpp::float2(-120.0f, 220.0f), 0.3f);
            m_ui.DestroyAfter(m_hint, 1.0f);
            m_hint = entt::null;
        }
        if(result.correct && m_match.Config().mode == Rules::MatchMode::Solo) {
            m_ui.Punch(m_seal, 0.22f);
        }

        // 連続正解の札（2連続から出し、途切れたら落とす）
        if(result.correct && result.streak >= 2) {
            if(m_streakTag == entt::null) {
                const PaperSpec spec = m_layout.Spec("streakTag");
                m_streakTag          = m_ui.Paper(spec);
                m_streakText         = m_ui.Text(L"", hlslpp::float2(6.0f, 2.0f), 0.6f, kSealRedColor, spec.sortOrder + 1, m_streakTag);
                m_ui.SlideIn(m_streakTag, hlslpp::float2(-200.0f, 0.0f));
            }
            m_ui.SetText(m_streakText, Number(result.streak) + L" 連続");
            m_ui.Punch(m_streakTag, 0.25f);
        } else if(!result.correct && m_streakTag != entt::null) {
            m_ui.SlideOut(m_streakTag, hlslpp::float2(0.0f, 160.0f), 0.2f, 0.5f);
            m_ui.DestroyAfter(m_streakTag, 0.8f);
            m_streakTag  = entt::null;
            m_streakText = entt::null;
        }
    }

}    // namespace ChronoSort::UI
