//----------------------------------------------------------------------------
//! @file   GameHud.cpp
//! @brief  対局画面の画面 UI の実装
//----------------------------------------------------------------------------
#include <ChronoSort/UI/GameHud.hpp>

#include <ChronoSort/Rules/GameRules.hpp>
#include <ChronoSort/Util/TextUtil.hpp>

#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIRectComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/WorldAnchorComponent.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UITweenUtility.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iterator>
#include <string>

namespace ChronoSort::UI {

    namespace {
        namespace BuiltIn = Tsukino::BuiltIn::ECS;
        using Tsukino::Core::Math::Easing::EaseType;

        constexpr float kDegToRad = 3.14159265f / 180.0f;

        // 描画順（大きいほど手前）
        constexpr int kLayerVignette = -50;
        constexpr int kLayerDesk     = 10;
        constexpr int kLayerAnchored = 40;
        constexpr int kLayerBanner   = 60;
        constexpr int kLayerResult   = 80;

        // ライフのインク壺の置き方（わざと揃えない）
        const hlslpp::float2 kInkPositions[] = {{262.0f, 150.0f}, {322.0f, 163.0f}, {378.0f, 146.0f}};
        const float          kInkAngles[]    = {-7.0f, 5.0f, -2.0f};

        //! 数字を全角にせず、そのままワイド文字列にします。
        std::wstring Number(long long value) {
            return std::to_wstring(value);
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! 作り先のシーン・アセット管理・表示する対局を受け取ります。
    //----------------------------------------------------------------------------
    GameHud::GameHud(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager, const Rules::Match& match)
        : m_scene(scene)
        , m_ui(scene, assetManager)
        , m_match(match) {
    }

    //----------------------------------------------------------------------------
    //! 常に出しておく UI を作ります。
    //----------------------------------------------------------------------------
    void GameHud::Build() {
        const bool solo = m_match.Config().mode == Rules::MatchMode::Solo;

        // 画面の四隅を暗くして、ランプに照らされた机に見せる
        {
            PaperSpec spec;
            spec.texture     = "vignette.png";
            spec.center      = hlslpp::float2(kReferenceWidth * 0.5f, kReferenceHeight * 0.5f);
            spec.size        = hlslpp::float2(kReferenceWidth, kReferenceHeight);
            spec.swayDegrees = 0.0f;
            spec.bobPixels   = 0.0f;
            spec.sortOrder   = kLayerVignette;
            const auto vignette = m_ui.Paper(spec);
            m_scene.GetRegistry().GetComponent<BuiltIn::SpriteComponent>(vignette).raycastTarget = false;
        }

        // 封蝋（ソロは得点、対戦は巡目）と、その札
        {
            PaperSpec label;
            label.texture      = "scrap_note.png";
            label.center       = hlslpp::float2(222.0f, 66.0f);
            label.size         = hlslpp::float2(150.0f, 52.0f);
            label.angleDegrees = 7.0f;
            label.sortOrder    = kLayerDesk;
            const auto tag     = m_ui.Paper(label);
            m_sealLabel        = m_ui.Text(solo ? L"得点" : L"巡目", hlslpp::float2(8.0f, 2.0f), 0.62f, kInkColor, kLayerDesk + 1, tag);
            m_ui.SlideIn(tag, hlslpp::float2(-260.0f, 0.0f), 0.2f);

            PaperSpec seal;
            seal.texture      = "wax_seal.png";
            seal.center       = hlslpp::float2(112.0f, 118.0f);
            seal.size         = hlslpp::float2(176.0f, 176.0f);
            seal.angleDegrees = -9.0f;
            seal.swayDegrees  = 0.5f;
            seal.sortOrder    = kLayerDesk + 2;
            m_seal            = m_ui.Paper(seal);
            m_sealText        = m_ui.Text(L"0", hlslpp::float2(0.0f, -2.0f), 1.15f, kParchmentColor, kLayerDesk + 3, m_seal);
            m_ui.SlideIn(m_seal, hlslpp::float2(-260.0f, -40.0f), 0.1f, 0.55f);
        }

        // ライフのインク壺（ソロ）
        if(solo) {
            for(int i = 0; i < Rules::kSoloLifeCount; ++i) {
                PaperSpec ink;
                ink.texture      = "ink_full.png";
                ink.center       = kInkPositions[i];
                ink.size         = hlslpp::float2(78.0f, 100.0f);
                ink.angleDegrees = kInkAngles[i];
                ink.swayDegrees  = 0.0f;
                ink.bobPixels    = 0.0f;
                ink.sortOrder    = kLayerDesk;
                m_inks.push_back(m_ui.Paper(ink));
                m_ui.SlideIn(m_inks.back(), hlslpp::float2(-200.0f, 0.0f), 0.3f + 0.08f * static_cast<float>(i));
            }
            m_shownLives = Rules::kSoloLifeCount;
        }

        // 山札の札（山札の 3D の近くにピンで留める）
        {
            PaperSpec tag;
            tag.texture      = "scrap_note.png";
            tag.center       = hlslpp::float2(1455.0f, 655.0f);
            tag.size         = hlslpp::float2(178.0f, 62.0f);
            tag.angleDegrees = 6.0f;
            tag.sortOrder    = kLayerDesk;
            m_deckTag        = m_ui.Paper(tag);
            m_deckText       = m_ui.Text(L"山札 0", hlslpp::float2(10.0f, 2.0f), 0.62f, kInkColor, kLayerDesk + 1, m_deckTag);

            PaperSpec pin;
            pin.texture     = "pin.png";
            pin.center      = hlslpp::float2(-70.0f, -18.0f);
            pin.size        = hlslpp::float2(40.0f, 40.0f);
            pin.swayDegrees = 0.0f;
            pin.bobPixels   = 0.0f;
            pin.sortOrder   = kLayerDesk + 2;
            pin.parent      = m_deckTag;
            m_ui.Paper(pin);
            m_ui.SlideIn(m_deckTag, hlslpp::float2(300.0f, 0.0f), 0.4f);
        }

        // 操作説明（最初の1枚を置いたら片付ける）
        {
            PaperSpec hint;
            hint.texture      = "scrap_wide.png";
            hint.center       = hlslpp::float2(262.0f, 812.0f);
            hint.size         = hlslpp::float2(440.0f, 120.0f);
            hint.angleDegrees = -3.0f;
            hint.sortOrder    = kLayerDesk;
            m_hint            = m_ui.Paper(hint);
            m_ui.Text(L"カードを掴んで、年代の合う隙間へ", hlslpp::float2(0.0f, -14.0f), 0.62f, kInkColor, kLayerDesk + 1, m_hint);
            m_ui.Text(L"クリックで選んでから置くこともできます", hlslpp::float2(0.0f, 20.0f), 0.46f, kFadedInkColor, kLayerDesk + 1, m_hint);

            PaperSpec tape;
            tape.texture      = "tape.png";
            tape.center       = hlslpp::float2(-170.0f, -50.0f);
            tape.size         = hlslpp::float2(120.0f, 38.0f);
            tape.angleDegrees = -24.0f;
            tape.swayDegrees  = 0.0f;
            tape.bobPixels    = 0.0f;
            tape.sortOrder    = kLayerDesk + 2;
            tape.parent       = m_hint;
            m_ui.Paper(tape);
            m_ui.SlideIn(m_hint, hlslpp::float2(0.0f, 200.0f), 1.2f);
        }

        // 各プレイヤーの名札。人間は左下、CPU は卓の奥の席の上に浮かべる
        const auto& players = m_match.Players();
        for(size_t i = 0; i < players.size(); ++i) {
            PaperSpec frame;
            frame.sortOrder = kLayerDesk;
            Tsukino::ECS::Entity tag;
            if(!players[i].isCpu) {
                frame.texture      = "scrap_note.png";
                frame.center       = hlslpp::float2(190.0f, 690.0f);
                frame.size         = hlslpp::float2(190.0f, 60.0f);
                frame.angleDegrees = 3.5f;
                tag                = m_ui.Paper(frame);
                m_ui.Text(Util::Utf8ToWide(players[i].name), hlslpp::float2(4.0f, 0.0f), 0.6f, kInkColor, kLayerDesk + 1, tag);
                m_handCounts.push_back(entt::null);
            } else {
                frame.texture      = "portrait_frame.png";
                frame.size         = hlslpp::float2(96.0f, 118.0f);
                frame.angleDegrees = m_ui.Jitter() * 6.0f;
                frame.center       = hlslpp::float2(0.0f, 0.0f);
                frame.sortOrder    = kLayerAnchored;
                tag                = m_ui.Paper(frame);

                // 卓の奥の席の上に貼り付ける（カメラが動いても席についてくる）
                auto& anchor                 = m_scene.GetRegistry().AddComponent<BuiltIn::WorldAnchorComponent>(tag);
                anchor.useFixedWorldPosition = true;
                const float spread           = (players.size() == 2) ? 0.0f : 320.0f;
                const float t                = (players.size() == 2) ? 0.0f : (static_cast<float>(i - 1) / static_cast<float>(players.size() - 2)) * 2.0f - 1.0f;
                anchor.fixedWorldPosition    = hlslpp::float3(t * spread, 0.0f, 300.0f - std::abs(t) * 30.0f);
                anchor.worldOffset           = hlslpp::float3(0.0f, 0.0f, -70.0f);
                anchor.screenOffset          = hlslpp::float2(0.0f, 30.0f);

                // 歴史家の肖像（取得できていなければ影絵）
                static const char* const kPortraits[] = {"portrait_herodotus.png", "portrait_sima_qian.png", "portrait_gibbon.png"};
                const std::string portrait = (i - 1 < std::size(kPortraits)) ? kPortraits[i - 1] : "";
                const bool        hasFace  = !portrait.empty() && std::filesystem::exists("ChronoSort/Assets/Textures/UI/" + portrait);

                PaperSpec face;
                face.texture     = hasFace ? portrait : "silhouette.png";
                face.center      = hlslpp::float2(0.0f, -13.0f);
                face.size        = hlslpp::float2(70.0f, 72.0f);
                face.swayDegrees = 0.0f;
                face.bobPixels   = 0.0f;
                face.sortOrder   = kLayerAnchored + 1;
                face.parent      = tag;
                m_ui.Paper(face);
                m_ui.Text(Util::Utf8ToWide(players[i].name), hlslpp::float2(0.0f, 41.0f), 0.42f, kInkColor, kLayerAnchored + 2, tag);
                m_handCounts.push_back(m_ui.Text(L"", hlslpp::float2(78.0f, 0.0f), 0.46f, kParchmentColor, kLayerAnchored + 2, tag, false));
            }
            m_playerTags.push_back(tag);
        }

        // 手番のリボン（名札にぶら下げる）
        {
            PaperSpec ribbon;
            ribbon.texture     = "ribbon.png";
            ribbon.size        = hlslpp::float2(30.0f, 100.0f);
            ribbon.swayDegrees = 3.0f;
            ribbon.swaySeconds = 2.2f;
            ribbon.sortOrder   = kLayerAnchored - 1;
            m_ribbon           = m_ui.Paper(ribbon);
        }
    }

    //----------------------------------------------------------------------------
    //! 値の反映と、時間切れの UI の片付けを行います。
    //----------------------------------------------------------------------------
    void GameHud::Update(float deltaTime) {
        const bool solo = m_match.Config().mode == Rules::MatchMode::Solo;

        // 時間切れの UI を片付ける
        for(auto it = m_expiring.begin(); it != m_expiring.end();) {
            it->first -= deltaTime;
            if(it->first <= 0.0f) {
                m_ui.Destroy(it->second);
                it = m_expiring.erase(it);
            } else {
                ++it;
            }
        }

        // 封蝋：得点は数え上げる。対戦は巡目
        if(solo) {
            const float target = static_cast<float>(m_match.Score());
            if(m_displayScore < target) {
                m_displayScore = std::min(target, m_displayScore + std::max(8.0f, (target - m_displayScore) * 6.0f) * deltaTime);
                m_ui.SetText(m_sealText, Number(static_cast<long long>(m_displayScore)));
            }
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

        // CPU の手札枚数
        const auto& players = m_match.Players();
        for(size_t i = 0; i < players.size() && i < m_handCounts.size(); ++i) {
            if(m_handCounts[i] == entt::null) {
                continue;
            }
            // 変わったときだけ書き換える（文字列を毎フレーム作り直さない）。脱落は -1 として扱う
            const int count = players[i].eliminated ? -1 : static_cast<int>(players[i].hand.size());
            if(i < m_shownHandCounts.size() && m_shownHandCounts[i] == count) {
                continue;
            }
            m_shownHandCounts.resize(std::max(m_shownHandCounts.size(), i + 1), -2);
            m_shownHandCounts[i] = count;
            m_ui.SetText(m_handCounts[i], count < 0 ? L"脱落" : (L"手札 " + Number(count)));
        }

        // 結果画面の得点の数え上げ
        if(m_resultTime >= 0.0f) {
            m_resultTime += deltaTime;
            if(m_resultScore != entt::null && solo) {
                const float t     = std::clamp((m_resultTime - 0.7f) / 1.2f, 0.0f, 1.0f);
                const float eased = 1.0f - (1.0f - t) * (1.0f - t);
                m_ui.SetText(m_resultScore, Number(static_cast<long long>(static_cast<float>(m_match.Score()) * eased)));
            }
        }
    }

    //----------------------------------------------------------------------------
    //! ライフのインク壺を更新します。
    //----------------------------------------------------------------------------
    void GameHud::RefreshLives() {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        const int               lives    = std::max(0, m_match.Lives());
        for(int i = lives; i < m_shownLives && i < static_cast<int>(m_inks.size()); ++i) {
            // 右のインク壺から倒れて空になる
            const Tsukino::ECS::Entity ink = m_inks[static_cast<size_t>(i)];
            m_ui.SetTexture(ink, "ink_empty.png");
            BuiltIn::UITween tip;
            tip.property  = BuiltIn::UITweenProperty::Rotation;
            tip.from      = 0.0f;
            tip.to        = 78.0f * kDegToRad;
            tip.duration  = 0.5f;
            tip.ease      = EaseType::OutBounce;
            tip.holdAtEnd = true;
            tip.tag       = 20;
            BuiltIn::UITweenUtility::Play(registry, ink, tip);

            BuiltIn::UITween slide;
            slide.property  = BuiltIn::UITweenProperty::OffsetY;
            slide.from      = 0.0f;
            slide.to        = 18.0f;
            slide.duration  = 0.5f;
            slide.ease      = EaseType::OutBounce;
            slide.holdAtEnd = true;
            slide.tag       = 21;
            BuiltIn::UITweenUtility::Play(registry, ink, slide);
        }
        m_shownLives = lives;
    }

    //----------------------------------------------------------------------------
    //! 手番のプレイヤーを示します。
    //----------------------------------------------------------------------------
    void GameHud::SetTurn(size_t player) {
        if(player >= m_playerTags.size() || m_ribbon == entt::null) {
            return;
        }
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        auto&                   transform = registry.GetComponent<BuiltIn::TransformComponent>(m_ribbon);
        transform.parent                  = m_playerTags[player];
        transform.position                = m_match.Players()[player].isCpu ? hlslpp::float3(40.0f, -34.0f, 0.0f) : hlslpp::float3(82.0f, -46.0f, 0.0f);
        transform.dirty                   = true;
        m_ui.SlideIn(m_ribbon, hlslpp::float2(0.0f, -60.0f), 0.0f, 0.4f);
        m_ui.Punch(m_playerTags[player], 0.12f);
    }

    //----------------------------------------------------------------------------
    //! 通知の紙片を滑り込ませ、少し置いてから抜きます。
    //----------------------------------------------------------------------------
    void GameHud::ShowBanner(const std::wstring& text, bool big) {
        PaperSpec spec;
        spec.texture      = "scrap_wide.png";
        spec.center       = hlslpp::float2(kReferenceWidth * 0.5f + m_ui.Jitter() * 30.0f, big ? 300.0f : 210.0f);
        spec.size         = big ? hlslpp::float2(660.0f, 180.0f) : hlslpp::float2(470.0f, 128.0f);
        spec.angleDegrees = m_ui.Jitter() * 4.0f;
        spec.swayDegrees  = 1.2f;
        spec.sortOrder    = kLayerBanner;
        const auto banner = m_ui.Paper(spec);
        m_ui.Text(text, hlslpp::float2(0.0f, 0.0f), big ? 1.7f : 1.15f, kInkColor, kLayerBanner + 1, banner);

        const float stay = big ? 2.0f : 1.1f;
        m_ui.SlideIn(banner, hlslpp::float2(-kReferenceWidth * 0.7f, 30.0f), 0.0f, 0.5f);
        m_ui.SlideOut(banner, hlslpp::float2(kReferenceWidth * 0.7f, -30.0f), 0.5f + stay, 0.4f);
        Expire(banner, 1.0f + stay);
    }

    //----------------------------------------------------------------------------
    //! 指定した座標に貼り付く UI を作ります。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity GameHud::CreateAnchor(Tsukino::ECS::Entity card, const hlslpp::float3& worldOffset) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        Tsukino::ECS::Entity    entity   = m_scene.CreateEntity();
        registry.AddComponent<BuiltIn::TransformComponent>(entity);
        auto& anchor       = registry.AddComponent<BuiltIn::WorldAnchorComponent>(entity);
        anchor.target      = card;
        anchor.worldOffset = worldOffset;
        return entity;
    }

    //----------------------------------------------------------------------------
    //! カードの上に判定の朱印を押します。
    //----------------------------------------------------------------------------
    void GameHud::ShowStamp(Tsukino::ECS::Entity card, bool correct) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        const auto              anchor   = CreateAnchor(card, hlslpp::float3(0.0f, 20.0f, 0.0f));

        PaperSpec spec;
        spec.texture      = correct ? "stamp_correct.png" : "stamp_wrong.png";
        spec.size         = hlslpp::float2(150.0f, 150.0f);
        spec.angleDegrees = m_ui.Jitter() * 14.0f;
        spec.swayDegrees  = 0.0f;
        spec.bobPixels    = 0.0f;
        spec.sortOrder    = kLayerAnchored + 5;
        spec.parent       = anchor;
        const auto stamp  = m_ui.Paper(spec);

        // 上から押し付ける：大きく薄い状態から一気に縮んで濃くなる
        BuiltIn::UITween press;
        press.property  = BuiltIn::UITweenProperty::Scale;
        press.from      = 2.4f;
        press.to        = 1.0f;
        press.duration  = 0.22f;
        press.ease      = EaseType::OutBack;
        press.holdAtEnd = true;
        press.tag       = 30;
        BuiltIn::UITweenUtility::Play(registry, stamp, press);

        BuiltIn::UITween ink;
        ink.property  = BuiltIn::UITweenProperty::Alpha;
        ink.from      = 0.0f;
        ink.to        = 1.0f;
        ink.duration  = 0.15f;
        ink.holdAtEnd = true;
        ink.tag       = 31;
        BuiltIn::UITweenUtility::Play(registry, stamp, ink);

        BuiltIn::UITween fade;
        fade.property  = BuiltIn::UITweenProperty::Alpha;
        fade.from      = 1.0f;
        fade.to        = 0.0f;
        fade.delay     = 1.1f;
        fade.duration  = 0.4f;
        fade.holdAtEnd = true;
        fade.tag       = 32;
        BuiltIn::UITweenUtility::Play(registry, anchor, fade);

        Expire(anchor, 1.6f);
    }

    //----------------------------------------------------------------------------
    //! カードの上に得点を浮き上がらせます。
    //----------------------------------------------------------------------------
    void GameHud::ShowScorePopup(Tsukino::ECS::Entity card, int score, bool closeCall, int streak) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        const auto              anchor   = CreateAnchor(card, hlslpp::float3(0.0f, 40.0f, 30.0f));

        float line = 0.0f;
        if(score > 0) {
            m_ui.Text(L"+" + Number(score), hlslpp::float2(70.0f, line), 1.1f, hlslpp::float4(1.0f, 0.86f, 0.45f, 1.0f), kLayerAnchored + 6, anchor);
            line += 36.0f;
        }
        if(closeCall) {
            m_ui.Text(L"僅差！", hlslpp::float2(70.0f, line), 0.7f, hlslpp::float4(1.0f, 0.95f, 0.8f, 1.0f), kLayerAnchored + 6, anchor);
            line += 28.0f;
        }
        if(streak >= 2) {
            m_ui.Text(Number(streak) + L"連続", hlslpp::float2(70.0f, line), 0.7f, hlslpp::float4(1.0f, 0.7f, 0.5f, 1.0f), kLayerAnchored + 6, anchor);
        }

        BuiltIn::UITween rise;
        rise.property  = BuiltIn::UITweenProperty::OffsetY;
        rise.from      = 10.0f;
        rise.to        = -60.0f;
        rise.duration  = 1.3f;
        rise.ease      = EaseType::OutCubic;
        rise.holdAtEnd = true;
        rise.tag       = 40;
        BuiltIn::UITweenUtility::Play(registry, anchor, rise);

        BuiltIn::UITween pop;
        pop.property = BuiltIn::UITweenProperty::Scale;
        pop.from     = 0.4f;
        pop.to       = 1.0f;
        pop.duration = 0.3f;
        pop.ease     = EaseType::OutBack;
        pop.holdAtEnd = true;
        pop.tag      = 41;
        BuiltIn::UITweenUtility::Play(registry, anchor, pop);

        BuiltIn::UITween fade;
        fade.property  = BuiltIn::UITweenProperty::Alpha;
        fade.from      = 1.0f;
        fade.to        = 0.0f;
        fade.delay     = 1.0f;
        fade.duration  = 0.4f;
        fade.holdAtEnd = true;
        fade.tag       = 42;
        BuiltIn::UITweenUtility::Play(registry, anchor, fade);

        Expire(anchor, 1.5f);
    }

    //----------------------------------------------------------------------------
    //! 1枚の判定が終わったことを伝えます。
    //----------------------------------------------------------------------------
    void GameHud::OnPlayResolved(size_t player, const Rules::PlayResult& result) {
        // 最初に人間が置いたら、操作説明は役目を終える
        if(player == 0 && m_hint != entt::null) {
            m_ui.SlideOut(m_hint, hlslpp::float2(-120.0f, 220.0f), 0.3f);
            Expire(m_hint, 1.0f);
            m_hint = entt::null;
        }
        if(result.correct && m_match.Config().mode == Rules::MatchMode::Solo) {
            m_ui.Punch(m_seal, 0.22f);
        }

        // 連続正解の札（2連続から出し、途切れたら落とす）
        if(result.correct && result.streak >= 2) {
            if(m_streakTag == entt::null) {
                PaperSpec spec;
                spec.texture      = "scrap_note.png";
                spec.center       = hlslpp::float2(150.0f, 236.0f);
                spec.size         = hlslpp::float2(160.0f, 54.0f);
                spec.angleDegrees = -5.0f;
                spec.sortOrder    = kLayerDesk + 4;
                m_streakTag       = m_ui.Paper(spec);
                m_streakText      = m_ui.Text(L"", hlslpp::float2(6.0f, 2.0f), 0.6f, kSealRedColor, kLayerDesk + 5, m_streakTag);
                m_ui.SlideIn(m_streakTag, hlslpp::float2(-200.0f, 0.0f));
            }
            m_ui.SetText(m_streakText, Number(result.streak) + L" 連続");
            m_ui.Punch(m_streakTag, 0.25f);
        } else if(!result.correct && m_streakTag != entt::null) {
            m_ui.SlideOut(m_streakTag, hlslpp::float2(0.0f, 160.0f), 0.2f, 0.5f);
            Expire(m_streakTag, 0.8f);
            m_streakTag  = entt::null;
            m_streakText = entt::null;
        }
    }

    //----------------------------------------------------------------------------
    //! 結果の羊皮紙を出します。
    //----------------------------------------------------------------------------
    void GameHud::ShowResult() {
        if(m_result != entt::null) {
            return;
        }
        const bool  solo    = m_match.Config().mode == Rules::MatchMode::Solo;
        const auto& players = m_match.Players();

        PaperSpec sheet;
        sheet.texture      = "scrap_result.png";
        sheet.center       = hlslpp::float2(kReferenceWidth * 0.5f, kReferenceHeight * 0.5f - 10.0f);
        sheet.size         = hlslpp::float2(680.0f, 540.0f);
        sheet.angleDegrees = -1.5f;
        sheet.swayDegrees  = 0.4f;
        sheet.sortOrder    = kLayerResult;
        m_result           = m_ui.Paper(sheet);
        m_ui.SlideIn(m_result, hlslpp::float2(0.0f, -kReferenceHeight), 0.0f, 0.7f);

        const int layer = kLayerResult + 1;
        m_ui.Text(L"記 録", hlslpp::float2(0.0f, -205.0f), 1.5f, kInkColor, layer, m_result);

        std::wstring rank = L"C";
        if(solo) {
            const int placed = players[0].correctCount;
            m_ui.Text(L"得点", hlslpp::float2(-240.0f, -110.0f), 0.8f, kFadedInkColor, layer, m_result, false);
            m_resultScore = m_ui.Text(L"0", hlslpp::float2(-60.0f, -110.0f), 1.3f, kInkColor, layer, m_result, false);
            m_ui.Text(L"並べた枚数", hlslpp::float2(-240.0f, -40.0f), 0.7f, kFadedInkColor, layer, m_result, false);
            m_ui.Text(Number(placed) + L" 枚", hlslpp::float2(-20.0f, -40.0f), 0.9f, kInkColor, layer, m_result, false);
            m_ui.Text(L"最長の連続", hlslpp::float2(-240.0f, 20.0f), 0.7f, kFadedInkColor, layer, m_result, false);
            m_ui.Text(Number(m_match.BestStreak()) + L" 連続", hlslpp::float2(-20.0f, 20.0f), 0.9f, kInkColor, layer, m_result, false);
            if(m_newRecord) {
                const auto badge = m_ui.Text(L"新記録", hlslpp::float2(-150.0f, -152.0f), 0.8f, kSealRedColor, layer + 1, m_result);
                BuiltIn::UITween blink;
                blink.property = BuiltIn::UITweenProperty::Scale;
                blink.from     = 0.92f;
                blink.to       = 1.12f;
                blink.duration = 0.5f;
                blink.ease     = EaseType::InOutSine;
                blink.loop     = BuiltIn::UITweenLoop::PingPong;
                blink.tag      = 52;
                BuiltIn::UITweenUtility::Play(m_scene.GetRegistry(), badge, blink);
            }
            if(m_match.Outcome() == Rules::MatchOutcome::SoloCleared || placed >= 30) {
                rank = L"S";
            } else if(placed >= 20) {
                rank = L"A";
            } else if(placed >= 10) {
                rank = L"B";
            }
        } else {
            const size_t winner = m_match.Winner();
            m_ui.Text(L"勝者", hlslpp::float2(-240.0f, -110.0f), 0.8f, kFadedInkColor, layer, m_result, false);
            m_ui.Text(Util::Utf8ToWide(players[winner].name), hlslpp::float2(-100.0f, -110.0f), 1.2f, kInkColor, layer, m_result, false);
            for(size_t i = 0; i < players.size(); ++i) {
                const float y = -40.0f + static_cast<float>(i) * 40.0f;
                m_ui.Text(Util::Utf8ToWide(players[i].name), hlslpp::float2(-240.0f, y), 0.62f, kFadedInkColor, layer, m_result, false);
                m_ui.Text(Number(players[i].correctCount) + L" 枚正解", hlslpp::float2(-20.0f, y), 0.62f, kInkColor, layer, m_result, false);
            }
            rank = winner == 0 ? L"勝" : L"敗";
        }

        // 印は少し遅れて押す
        {
            PaperSpec seal;
            seal.texture      = "wax_seal.png";
            seal.center       = hlslpp::float2(200.0f, -40.0f);
            seal.size         = hlslpp::float2(150.0f, 150.0f);
            seal.angleDegrees = 12.0f;
            seal.swayDegrees  = 0.0f;
            seal.sortOrder    = layer + 2;
            seal.parent       = m_result;
            const auto stamp  = m_ui.Paper(seal);
            m_ui.Text(rank, hlslpp::float2(0.0f, -2.0f), 1.8f, kParchmentColor, layer + 3, stamp);

            BuiltIn::UITween press;
            press.property  = BuiltIn::UITweenProperty::Scale;
            press.from      = 2.6f;
            press.to        = 1.0f;
            press.delay     = 2.0f;
            press.duration  = 0.25f;
            press.ease      = EaseType::OutBack;
            press.holdAtEnd = true;
            press.tag       = 50;
            BuiltIn::UITweenUtility::Play(m_scene.GetRegistry(), stamp, press);
            BuiltIn::UITween appear;
            appear.property  = BuiltIn::UITweenProperty::Alpha;
            appear.from      = 0.0f;
            appear.to        = 1.0f;
            appear.delay     = 2.0f;
            appear.duration  = 0.1f;
            appear.holdAtEnd = true;
            appear.tag       = 51;
            BuiltIn::UITweenUtility::Play(m_scene.GetRegistry(), stamp, appear);
        }

        // ボタン：ピンで留めたメモ紙。わざと高さも傾きも揃えない
        auto makeButton = [&](const std::wstring& label, const hlslpp::float2& at, float angle) {
            PaperSpec spec;
            spec.texture      = "scrap_note.png";
            spec.center       = at;
            spec.size         = hlslpp::float2(220.0f, 74.0f);
            spec.angleDegrees = angle;
            spec.swayDegrees  = 1.0f;
            spec.sortOrder    = layer + 4;
            spec.parent       = m_result;
            const auto button = m_ui.Paper(spec);
            m_ui.Text(label, hlslpp::float2(8.0f, 2.0f), 0.75f, kInkColor, layer + 5, button);
            PaperSpec pin;
            pin.texture     = "pin.png";
            pin.center      = hlslpp::float2(-88.0f, -22.0f);
            pin.size        = hlslpp::float2(36.0f, 36.0f);
            pin.swayDegrees = 0.0f;
            pin.bobPixels   = 0.0f;
            pin.sortOrder   = layer + 6;
            pin.parent      = button;
            m_ui.Paper(pin);
            m_ui.MakeButton(button);
            return button;
        };
        m_retryButton = makeButton(L"もう一度", hlslpp::float2(-140.0f, 170.0f), -4.0f);
        m_titleButton = makeButton(L"タイトルへ", hlslpp::float2(150.0f, 182.0f), 3.0f);
        m_resultTime  = 0.0f;
    }

    //----------------------------------------------------------------------------
    //! 結果画面で選ばれた行動を返します。
    //----------------------------------------------------------------------------
    ResultChoice GameHud::PollResultChoice() {
        // 羊皮紙が落ちてくる途中の誤クリックは受け付けない
        if(m_result == entt::null || m_resultTime < 0.8f) {
            return ResultChoice::None;
        }
        if(m_ui.Clicked(m_retryButton)) {
            return ResultChoice::Retry;
        }
        if(m_ui.Clicked(m_titleButton)) {
            return ResultChoice::Title;
        }
        return ResultChoice::None;
    }

    //----------------------------------------------------------------------------
    //! 一定時間後に破棄する UI を登録します。
    //----------------------------------------------------------------------------
    void GameHud::Expire(Tsukino::ECS::Entity entity, float seconds) {
        m_expiring.emplace_back(seconds, entity);
    }

}    // namespace ChronoSort::UI
