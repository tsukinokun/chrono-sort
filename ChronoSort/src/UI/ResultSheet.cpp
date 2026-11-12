//----------------------------------------------------------------------------
//! @file   ResultSheet.cpp
//! @brief  対局の結果の羊皮紙の実装
//----------------------------------------------------------------------------
#include <ChronoSort/UI/ResultSheet.hpp>

#include <ChronoSort/ECS/Component/CountUpTextComponent.hpp>
#include <ChronoSort/Match/MatchEvents.hpp>
#include <ChronoSort/Match/MatchSession.hpp>
#include <ChronoSort/UI/TweenTags.hpp>
#include <ChronoSort/UI/Widgets.hpp>
#include <ChronoSort/Util/AssetPaths.hpp>
#include <ChronoSort/Util/Localization.hpp>
#include <ChronoSort/Util/TextUtil.hpp>

#include <Tsukino/BuiltIn/ECS/UI/UITweenUtility.hpp>
#include <Tsukino/Core/ECS/Event/EventBus.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>

#include <string>

namespace ChronoSort::UI {

    namespace {
        namespace BuiltIn = Tsukino::BuiltIn::ECS;
        using Tsukino::Core::Math::Easing::EaseType;

        constexpr float kClickGuard     = 0.8f;    // 紙が落ちてくる途中の誤クリックを受け付けない時間（秒）
        constexpr float kCountUpDelay   = 0.7f;    // 得点を数え上げ始めるまで（秒）
        constexpr float kCountUpSeconds = 1.2f;    // 得点を数え上げる長さ（秒）
        constexpr float kSealDelay      = 2.0f;    // ランクの印を押すまで（秒）
    }    // namespace

    //----------------------------------------------------------------------------
    //! 作り先のシーン・アセット管理・表示する対局を受け取り、出来事の購読を始めます。
    //----------------------------------------------------------------------------
    ResultSheet::ResultSheet(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager, const Rules::Match& match)
        : m_scene(scene)
        , m_ui(scene, assetManager)
        , m_match(match)
        , m_layout(Util::AssetPaths::UILayout("GameHud")) {
        Tsukino::ECS::EventBus& events = scene.GetEventBus();
        m_connections.push_back(events.Subscribe<Match::MatchEndedEvent>([this](const Match::MatchEndedEvent& e) {
            m_newRecord = m_match.Config().mode == Rules::MatchMode::Solo && e.newRecord;
        }));
        m_connections.push_back(events.Subscribe<Match::ResultRequestedEvent>([this](const Match::ResultRequestedEvent&) { Show(); }));
    }

    //----------------------------------------------------------------------------
    //! 結果の羊皮紙を出します。
    //----------------------------------------------------------------------------
    void ResultSheet::Show() {
        if(m_sheet != entt::null) {
            return;
        }
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        const bool              solo     = m_match.Config().mode == Rules::MatchMode::Solo;
        const auto&             players  = m_match.Players();

        const PaperSpec sheet = m_layout.Spec("resultSheet");
        m_sheet               = m_ui.Paper(sheet);
        m_ui.SlideIn(m_sheet, hlslpp::float2(0.0f, -kReferenceHeight), 0.0f, 0.7f);

        const int layer = sheet.sortOrder + 1;
        m_ui.Text(Util::Text("result.title"), hlslpp::float2(0.0f, -205.0f), 1.5f, kInkColor, layer, m_sheet);

        std::wstring rank;
        if(solo) {
            const int placed = players[0].correctCount;
            m_ui.Text(Util::Text("result.score"), hlslpp::float2(-240.0f, -110.0f), 0.8f, kFadedInkColor, layer, m_sheet, false);
            const auto score = m_ui.Text(L"0", hlslpp::float2(-60.0f, -110.0f), 1.3f, kInkColor, layer, m_sheet, false);
            auto&      count = registry.AddComponent<ECS::CountUpTextComponent>(score);
            count.target     = static_cast<float>(m_match.Score());
            count.delay      = kCountUpDelay;
            count.duration   = kCountUpSeconds;

            m_ui.Text(Util::Text("result.placed"), hlslpp::float2(-240.0f, -40.0f), 0.7f, kFadedInkColor, layer, m_sheet, false);
            m_ui.Text(Util::Format("result.cards", {std::to_wstring(placed)}), hlslpp::float2(-20.0f, -40.0f), 0.9f, kInkColor, layer, m_sheet, false);
            m_ui.Text(Util::Text("result.bestStreak"), hlslpp::float2(-240.0f, 20.0f), 0.7f, kFadedInkColor, layer, m_sheet, false);
            m_ui.Text(Util::Format("result.streak", {std::to_wstring(m_match.BestStreak())}), hlslpp::float2(-20.0f, 20.0f), 0.9f, kInkColor, layer, m_sheet, false);
            if(m_newRecord) {
                const auto       badge = m_ui.Text(Util::Text("result.newRecord"), hlslpp::float2(-150.0f, -152.0f), 0.8f, kSealRedColor, layer + 1, m_sheet);
                BuiltIn::UITween blink;
                blink.property = BuiltIn::UITweenProperty::Scale;
                blink.from     = 0.92f;
                blink.to       = 1.12f;
                blink.duration = 0.5f;
                blink.ease     = EaseType::InOutSine;
                blink.loop     = BuiltIn::UITweenLoop::PingPong;
                blink.tag      = Tag(TweenTag::NewRecordBlink);
                BuiltIn::UITweenUtility::Play(registry, badge, blink);
            }
            rank = Util::Utf8ToWide(Rules::SoloRank(m_match));
        } else {
            const size_t winner = m_match.Winner();
            m_ui.Text(Util::Text("result.winner"), hlslpp::float2(-240.0f, -110.0f), 0.8f, kFadedInkColor, layer, m_sheet, false);
            m_ui.Text(Util::Text(players[winner].name), hlslpp::float2(-100.0f, -110.0f), 1.2f, kInkColor, layer, m_sheet, false);
            for(size_t i = 0; i < players.size(); ++i) {
                const float y = -40.0f + static_cast<float>(i) * 40.0f;
                m_ui.Text(Util::Text(players[i].name), hlslpp::float2(-240.0f, y), 0.62f, kFadedInkColor, layer, m_sheet, false);
                m_ui.Text(Util::Format("result.correct", {std::to_wstring(players[i].correctCount)}), hlslpp::float2(-20.0f, y), 0.62f, kInkColor, layer, m_sheet, false);
            }
            rank = Util::Text(winner == 0 ? "result.win" : "result.lose");
        }

        // 印は少し遅れて押す
        {
            PaperSpec seal   = m_layout.Spec("resultSeal");
            seal.parent      = m_sheet;
            const auto stamp = m_ui.Paper(seal);
            m_ui.Text(rank, hlslpp::float2(0.0f, -2.0f), 1.8f, kParchmentColor, seal.sortOrder + 1, stamp);

            BuiltIn::UITween press;
            press.property  = BuiltIn::UITweenProperty::Scale;
            press.from      = 2.6f;
            press.to        = 1.0f;
            press.delay     = kSealDelay;
            press.duration  = 0.25f;
            press.ease      = EaseType::OutBack;
            press.holdAtEnd = true;
            press.tag       = Tag(TweenTag::ResultSealPress);
            BuiltIn::UITweenUtility::Play(registry, stamp, press);
            BuiltIn::UITween appear;
            appear.property  = BuiltIn::UITweenProperty::Alpha;
            appear.from      = 0.0f;
            appear.to        = 1.0f;
            appear.delay     = kSealDelay;
            appear.duration  = 0.1f;
            appear.holdAtEnd = true;
            appear.tag       = Tag(TweenTag::ResultSealAppear);
            BuiltIn::UITweenUtility::Play(registry, stamp, appear);
        }

        // ボタン：ピンで留めたメモ紙。わざと高さも傾きも揃えない
        auto makeButton = [&](const char* name, const std::wstring& label) {
            PaperSpec spec    = m_layout.Spec(name);
            spec.parent       = m_sheet;
            const auto button = NoteButton(m_ui, spec, label, 0.75f, hlslpp::float2(8.0f, 2.0f));
            PaperSpec pin     = m_layout.Spec("resultPin");
            pin.parent        = button;
            m_ui.Paper(pin);
            return button;
        };
        m_retryButton = makeButton("resultRetry", Util::Text("result.retry"));
        m_cardsButton = makeButton("resultCards", Util::Text("result.cards.button"));
        m_titleButton = makeButton("resultTitle", Util::Text("common.toTitle"));

        // 羊皮紙が落ちてくる途中の誤クリックは受け付けない（対局の時間で数えるので、一時停止中は進まない）
        Match::Session(registry).sequencer.Schedule(kClickGuard, [this] { m_ready = true; });
    }

    //----------------------------------------------------------------------------
    //! 選ばれた行動を返します。
    //----------------------------------------------------------------------------
    ResultChoice ResultSheet::Poll() {
        if(!m_ready) {
            return ResultChoice::None;
        }
        if(m_ui.Clicked(m_retryButton)) {
            return ResultChoice::Retry;
        }
        if(m_ui.Clicked(m_titleButton)) {
            return ResultChoice::Title;
        }
        if(m_ui.Clicked(m_cardsButton)) {
            return ResultChoice::Cards;
        }
        return ResultChoice::None;
    }

}    // namespace ChronoSort::UI
