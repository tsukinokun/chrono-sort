//----------------------------------------------------------------------------
//! @file   ResultSheet.cpp
//! @brief  対局の結果の羊皮紙の実装
//----------------------------------------------------------------------------
#include <ChronoSort/UI/ResultSheet.hpp>

#include <ChronoSort/ECS/Component/CountUpTextComponent.hpp>
#include <ChronoSort/Match/MatchEvents.hpp>
#include <ChronoSort/Match/MatchSession.hpp>
#include <ChronoSort/Util/TextUtil.hpp>

#include <Tsukino/BuiltIn/ECS/UI/UITweenUtility.hpp>
#include <Tsukino/Core/ECS/Event/EventBus.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>

#include <string>

namespace ChronoSort::UI {

    namespace {
        namespace BuiltIn = Tsukino::BuiltIn::ECS;
        using Tsukino::Core::Math::Easing::EaseType;

        constexpr int   kLayerResult    = 80;      // 結果の紙の描画順
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
        , m_match(match) {
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

        PaperSpec sheet;
        sheet.texture      = "scrap_result.png";
        sheet.center       = hlslpp::float2(kReferenceWidth * 0.5f, kReferenceHeight * 0.5f - 10.0f);
        sheet.size         = hlslpp::float2(680.0f, 540.0f);
        sheet.angleDegrees = -1.5f;
        sheet.swayDegrees  = 0.4f;
        sheet.sortOrder    = kLayerResult;
        m_sheet            = m_ui.Paper(sheet);
        m_ui.SlideIn(m_sheet, hlslpp::float2(0.0f, -kReferenceHeight), 0.0f, 0.7f);

        const int layer = kLayerResult + 1;
        m_ui.Text(L"記 録", hlslpp::float2(0.0f, -205.0f), 1.5f, kInkColor, layer, m_sheet);

        std::wstring rank;
        if(solo) {
            const int placed = players[0].correctCount;
            m_ui.Text(L"得点", hlslpp::float2(-240.0f, -110.0f), 0.8f, kFadedInkColor, layer, m_sheet, false);
            const auto score = m_ui.Text(L"0", hlslpp::float2(-60.0f, -110.0f), 1.3f, kInkColor, layer, m_sheet, false);
            auto&      count = registry.AddComponent<ECS::CountUpTextComponent>(score);
            count.target     = static_cast<float>(m_match.Score());
            count.delay      = kCountUpDelay;
            count.duration   = kCountUpSeconds;

            m_ui.Text(L"並べた枚数", hlslpp::float2(-240.0f, -40.0f), 0.7f, kFadedInkColor, layer, m_sheet, false);
            m_ui.Text(std::to_wstring(placed) + L" 枚", hlslpp::float2(-20.0f, -40.0f), 0.9f, kInkColor, layer, m_sheet, false);
            m_ui.Text(L"最長の連続", hlslpp::float2(-240.0f, 20.0f), 0.7f, kFadedInkColor, layer, m_sheet, false);
            m_ui.Text(std::to_wstring(m_match.BestStreak()) + L" 連続", hlslpp::float2(-20.0f, 20.0f), 0.9f, kInkColor, layer, m_sheet, false);
            if(m_newRecord) {
                const auto       badge = m_ui.Text(L"新記録", hlslpp::float2(-150.0f, -152.0f), 0.8f, kSealRedColor, layer + 1, m_sheet);
                BuiltIn::UITween blink;
                blink.property = BuiltIn::UITweenProperty::Scale;
                blink.from     = 0.92f;
                blink.to       = 1.12f;
                blink.duration = 0.5f;
                blink.ease     = EaseType::InOutSine;
                blink.loop     = BuiltIn::UITweenLoop::PingPong;
                blink.tag      = 52;
                BuiltIn::UITweenUtility::Play(registry, badge, blink);
            }
            rank = Util::Utf8ToWide(Rules::SoloRank(m_match));
        } else {
            const size_t winner = m_match.Winner();
            m_ui.Text(L"勝者", hlslpp::float2(-240.0f, -110.0f), 0.8f, kFadedInkColor, layer, m_sheet, false);
            m_ui.Text(Util::Utf8ToWide(players[winner].name), hlslpp::float2(-100.0f, -110.0f), 1.2f, kInkColor, layer, m_sheet, false);
            for(size_t i = 0; i < players.size(); ++i) {
                const float y = -40.0f + static_cast<float>(i) * 40.0f;
                m_ui.Text(Util::Utf8ToWide(players[i].name), hlslpp::float2(-240.0f, y), 0.62f, kFadedInkColor, layer, m_sheet, false);
                m_ui.Text(std::to_wstring(players[i].correctCount) + L" 枚正解", hlslpp::float2(-20.0f, y), 0.62f, kInkColor, layer, m_sheet, false);
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
            seal.parent       = m_sheet;
            const auto stamp  = m_ui.Paper(seal);
            m_ui.Text(rank, hlslpp::float2(0.0f, -2.0f), 1.8f, kParchmentColor, layer + 3, stamp);

            BuiltIn::UITween press;
            press.property  = BuiltIn::UITweenProperty::Scale;
            press.from      = 2.6f;
            press.to        = 1.0f;
            press.delay     = kSealDelay;
            press.duration  = 0.25f;
            press.ease      = EaseType::OutBack;
            press.holdAtEnd = true;
            press.tag       = 50;
            BuiltIn::UITweenUtility::Play(registry, stamp, press);
            BuiltIn::UITween appear;
            appear.property  = BuiltIn::UITweenProperty::Alpha;
            appear.from      = 0.0f;
            appear.to        = 1.0f;
            appear.delay     = kSealDelay;
            appear.duration  = 0.1f;
            appear.holdAtEnd = true;
            appear.tag       = 51;
            BuiltIn::UITweenUtility::Play(registry, stamp, appear);
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
            spec.parent       = m_sheet;
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
        return ResultChoice::None;
    }

}    // namespace ChronoSort::UI
