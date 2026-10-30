//----------------------------------------------------------------------------
//! @file   TutorialGuide.cpp
//! @brief  慣れるまでの対局で出す、遊び方の説明と手本の実装
//----------------------------------------------------------------------------
#include <ChronoSort/UI/TutorialGuide.hpp>

#include <ChronoSort/Board/BoardView.hpp>
#include <ChronoSort/Board/CardFactory.hpp>
#include <ChronoSort/Game/GameServices.hpp>
#include <ChronoSort/Game/TuningSet.hpp>
#include <ChronoSort/Match/MatchEvents.hpp>
#include <ChronoSort/Match/MatchSession.hpp>
#include <ChronoSort/UI/TweenTags.hpp>
#include <ChronoSort/UI/Widgets.hpp>
#include <ChronoSort/Util/AssetPaths.hpp>

#include <Tsukino/BuiltIn/ECS/Component/PointerTargetComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UICanvasGroupComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/WorldAnchorComponent.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UITweenUtility.hpp>
#include <Tsukino/Core/ECS/Event/EventBus.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>

#include <algorithm>
#include <cmath>
#include <string>

namespace ChronoSort::UI {

    namespace {
        namespace BuiltIn = Tsukino::BuiltIn::ECS;
        using Tsukino::Core::Math::Easing::EaseType;

        constexpr float kOkGuard     = 0.6f;     // 紙が出てから「わかった」を受け付けるまで（秒）
        constexpr float kDimAlpha    = 0.55f;    // 暗幕の濃さ
        constexpr float kFadeSeconds = 0.25f;    // 手本の現れ・消えの時間（秒）
        constexpr float kCardAlpha   = 0.7f;     // 手本のカードの濃さ
        const hlslpp::float2 kHintCenter = hlslpp::float2(262.0f, 812.0f);    // 左下の小さな説明の位置（GameHud.json の hint）
        const hlslpp::float2 kPointerOffset = hlslpp::float2(26.0f, 52.0f);   // カードの中心から、指先の光まで
        const hlslpp::float2 kLabelOffset   = hlslpp::float2(0.0f, -128.0f);  // カードの中心から、手本に添える札まで

        //! 0〜1 を、ゆっくり始まりゆっくり終わる 0〜1 にします。
        float EaseInOut(float t) {
            return 0.5f - 0.5f * std::cos(std::clamp(t, 0.0f, 1.0f) * 3.14159265f);
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! 作り先のシーンとアセット管理を受け取り、出来事の購読を始めます。
    //----------------------------------------------------------------------------
    TutorialGuide::TutorialGuide(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager)
        : m_ui(scene, assetManager)
        , m_layout(Util::AssetPaths::UILayout("Tutorial")) {
        Tsukino::ECS::EventBus& events = scene.GetEventBus();
        // 最初の人間の手番（配り終えて、手札が並び終えた後）に遊び方の紙を出す
        m_connections.push_back(events.Subscribe<Match::TurnStartedEvent>([this](const Match::TurnStartedEvent& e) {
            if(e.human && !m_shown && Match::Session(m_ui.Scene().GetRegistry()).tutorial) {
                Open();
            }
        }));
        // 放置されたら、もう一度手本を見せる
        m_connections.push_back(events.Subscribe<Match::IdleNudgeEvent>([this](const Match::IdleNudgeEvent& e) {
            if(e.active && m_dismissed) {
                StartDemo();
            }
        }));
    }

    //----------------------------------------------------------------------------
    //! 遊び方の紙を出します。
    //----------------------------------------------------------------------------
    void TutorialGuide::Open() {
        m_shown = true;
        Tsukino::ECS::Registry& registry = m_ui.Scene().GetRegistry();
        const Match::MatchSession& session = Match::Session(registry);
        const bool                 solo    = session.config.mode == Rules::MatchMode::Solo;

        // 暗幕は下の卓や札へのクリックも受け止める
        m_dim = m_ui.Paper(m_layout.Spec("dim"));
        registry.GetComponent<BuiltIn::SpriteComponent>(m_dim).tintColor = hlslpp::float4(0.0f, 0.0f, 0.0f, kDimAlpha);
        registry.AddComponent<BuiltIn::PointerTargetComponent>(m_dim);
        BuiltIn::UITweenUtility::FadeIn(registry, m_dim, 0.0f, 0.3f);

        const PaperSpec sheet = m_layout.Spec("sheet");
        m_sheet               = m_ui.Paper(sheet);
        m_ui.SlideIn(m_sheet, hlslpp::float2(0.0f, -kReferenceHeight), 0.05f, 0.55f);
        const int layer = sheet.sortOrder + 1;

        m_ui.Text(L"遊び方", hlslpp::float2(0.0f, -222.0f), 1.15f, kInkColor, layer, m_sheet);
        const float x    = -380.0f;
        auto        item = [&](const std::wstring& mark, const std::wstring& text, float y) {
            m_ui.Text(mark, hlslpp::float2(x, y), 0.66f, kSealRedColor, layer, m_sheet, false);
            m_ui.Text(text, hlslpp::float2(x + 44.0f, y), 0.62f, kInkColor, layer, m_sheet, false);
        };
        auto note = [&](const std::wstring& text, float y) { m_ui.Text(text, hlslpp::float2(x + 44.0f, y), 0.48f, kFadedInkColor, layer, m_sheet, false); };

        item(L"一", L"手札のカードを掴んで、場の列の、年代が合う隙間で離す", -142.0f);
        note(L"クリックで選んでから、隙間をクリックしても置けます", -104.0f);
        item(L"二", L"年代は置いた後に明かされる", -48.0f);
        if(solo) {
            note(L"外れるとインクが1つ減ります。インクが尽きると終わり", -10.0f);
        } else {
            note(L"外れたカードは捨て札になり、1枚引きます。先に出し切った人の勝ち", -10.0f);
        }
        item(L"三", L"ホイールで場の列を左右に見渡せる", 46.0f);
        note(L"Esc で一時停止", 84.0f);

        PaperSpec ok = m_layout.Spec("ok");
        ok.parent    = m_sheet;
        m_ok         = NoteButton(m_ui, ok, L"わかった", 0.8f);
        PaperSpec pin = m_layout.Spec("okPin");
        pin.parent    = m_ok;
        m_ui.Paper(pin);
        m_okGuard = kOkGuard;
    }

    //----------------------------------------------------------------------------
    //! 遊び方の紙を閉じます。
    //----------------------------------------------------------------------------
    void TutorialGuide::Dismiss() {
        if(m_sheet == entt::null) {
            return;
        }
        Tsukino::ECS::Registry& registry = m_ui.Scene().GetRegistry();
        Game::Services().Sounds().Play("ui.close");

        // 紙は縮みながら左下の小さな説明の位置へ飛んで消える（そこに DeskHud が小さな説明を残す）
        const PaperSpec  sheet = m_layout.Spec("sheet");
        BuiltIn::UITween shrink;
        shrink.property  = BuiltIn::UITweenProperty::Scale;
        shrink.from      = 1.0f;
        shrink.to        = 0.3f;
        shrink.duration  = 0.45f;
        shrink.ease      = EaseType::InCubic;
        shrink.holdAtEnd = true;
        shrink.tag       = Tag(TweenTag::TutorialShrink);
        BuiltIn::UITweenUtility::Play(registry, m_sheet, shrink);
        m_ui.SlideOut(m_sheet, kHintCenter - sheet.center, 0.0f, 0.45f);
        m_ui.DestroyAfter(m_sheet, 0.5f);

        BuiltIn::UITweenUtility::FadeOut(registry, m_dim, 0.0f, 0.3f);
        m_ui.DestroyAfter(m_dim, 0.35f);
        // 消えるまでの間にクリックを吸わないよう、暗幕の当たり判定は先に外す
        registry.GetComponent<BuiltIn::SpriteComponent>(m_dim).raycastTarget = false;

        m_sheet = m_dim = m_ok = entt::null;
        m_dismissed = true;
        Match::Events(registry).Publish(Match::TutorialDismissedEvent{});
        StartDemo();
    }

    //----------------------------------------------------------------------------
    //! 「わかった」の札と Enter・Space を受け、手本を動かします。
    //----------------------------------------------------------------------------
    void TutorialGuide::Update(const Tsukino::Input::InputSystem& input, float deltaTime) {
        Tsukino::ECS::Registry&    registry = m_ui.Scene().GetRegistry();
        const Match::MatchSession& session  = Match::Session(registry);

        if(m_sheet != entt::null) {
            m_okGuard -= deltaTime;
            if(m_okGuard <= 0.0f &&
               (m_ui.Clicked(m_ok) || input.IsKeyPressed(Tsukino::Input::KeyCode::Enter) || input.IsKeyPressed(Tsukino::Input::KeyCode::Space))) {
                Dismiss();
            }
            return;
        }
        if(m_card == entt::null) {
            return;
        }

        // 掴む・選ぶ・手番が終わる、のどれかで手本は役目を終える（放置されたらまた出す）
        const bool busy = session.input.grabbed >= 0 || session.input.selected >= 0;
        if(busy || session.phase != Match::MatchPhase::HumanTurn) {
            StopDemo();
            return;
        }
        if(session.paused) {
            return;
        }

        const Match::InputTuning& t     = Game::Tuning().Input();
        const float               total = kFadeSeconds + t.demoMoveSeconds + t.demoHoldSeconds + kFadeSeconds + t.demoRestSeconds;
        m_time += deltaTime;
        if(m_time >= total) {
            ++m_cycle;
            if(!PrepareDemoCycle()) {
                StopDemo();
                return;
            }
        }

        // 見えない2点（手札のカードと隙間に貼り付く）の、画面の位置を結んで運ぶ
        const hlslpp::float2 from = registry.GetComponent<BuiltIn::TransformComponent>(m_from).position.xy;
        const hlslpp::float2 to   = registry.GetComponent<BuiltIn::TransformComponent>(m_to).position.xy;
        const float          move = (m_time - kFadeSeconds) / t.demoMoveSeconds;
        const float          u    = EaseInOut(move);
        hlslpp::float2       at   = from + (to - from) * u;
        at.y -= t.demoArc * std::sin(std::clamp(move, 0.0f, 1.0f) * 3.14159265f);

        float alpha = 1.0f;
        if(m_time < kFadeSeconds) {
            alpha = m_time / kFadeSeconds;
        } else {
            const float fadeStart = kFadeSeconds + t.demoMoveSeconds + t.demoHoldSeconds;
            alpha                 = 1.0f - std::clamp((m_time - fadeStart) / kFadeSeconds, 0.0f, 1.0f);
        }
        auto& transform    = registry.GetComponent<BuiltIn::TransformComponent>(m_card);
        transform.position = hlslpp::float3(at, 0.0f);
        transform.dirty    = true;
        // 見えない点の行き先を変えた直後は、まだ画面へ写されていない（WorldAnchorSystem は対局の更新の中で動く）ので出さない
        const bool anchored = m_settle <= 0 && registry.GetComponent<BuiltIn::WorldAnchorComponent>(m_from).visible &&
                              registry.GetComponent<BuiltIn::WorldAnchorComponent>(m_to).visible;
        registry.GetComponent<BuiltIn::UICanvasGroupComponent>(m_card).alpha = anchored ? alpha : 0.0f;
        --m_settle;
    }

    //----------------------------------------------------------------------------
    //! 手本を始めます。
    //----------------------------------------------------------------------------
    void TutorialGuide::StartDemo() {
        if(m_card != entt::null) {
            return;
        }
        Tsukino::ECS::Registry& registry = m_ui.Scene().GetRegistry();
        const Match::MatchSession& session = Match::Session(registry);
        if(!session.tutorial || session.phase != Match::MatchPhase::HumanTurn || session.input.grabbed >= 0 || session.input.selected >= 0) {
            return;
        }

        // 見えない2点。WorldAnchorSystem が 3D の位置を画面へ写して、Transform に書いてくれる
        auto anchor = [&]() {
            const Tsukino::ECS::Entity entity = m_ui.Scene().CreateEntity();
            registry.AddComponent<BuiltIn::TransformComponent>(entity);
            registry.AddComponent<BuiltIn::WorldAnchorComponent>(entity);
            return entity;
        };
        m_from = anchor();
        m_to   = anchor();

        // 半透明のカード。指先の光と添える札はカードの子にして一緒に運び、カードの透明度でまとめて薄くする
        PaperSpec card = m_layout.Spec("ghostCard");
        m_card         = m_ui.Paper(card);    // 絵は PrepareDemoCycle で、運ぶ手札のカードの面にする
        registry.GetComponent<BuiltIn::SpriteComponent>(m_card).tintColor     = hlslpp::float4(1.0f, 1.0f, 1.0f, kCardAlpha);
        registry.GetComponent<BuiltIn::SpriteComponent>(m_card).raycastTarget = false;
        registry.AddComponent<BuiltIn::UICanvasGroupComponent>(m_card).alpha  = 0.0f;

        PaperSpec pointer = m_layout.Spec("ghostPointer");
        pointer.center    = kPointerOffset;
        pointer.parent    = m_card;
        m_pointer         = m_ui.Paper(pointer);
        registry.GetComponent<BuiltIn::SpriteComponent>(m_pointer).tintColor     = hlslpp::float4(1.0f, 0.93f, 0.75f, 0.95f);
        registry.GetComponent<BuiltIn::SpriteComponent>(m_pointer).raycastTarget = false;

        PaperSpec label = m_layout.Spec("ghostLabel");
        label.center    = kLabelOffset;
        label.parent    = m_card;
        m_label         = m_ui.Paper(label);
        registry.GetComponent<BuiltIn::SpriteComponent>(m_label).raycastTarget = false;
        m_ui.Text(L"掴んで、年代の合う隙間へ", hlslpp::float2(4.0f, 1.0f), 0.5f, kInkColor, label.sortOrder + 1, m_label);

        if(!PrepareDemoCycle()) {
            StopDemo();
        }
    }

    //----------------------------------------------------------------------------
    //! 手本を片付けます。
    //----------------------------------------------------------------------------
    void TutorialGuide::StopDemo() {
        if(m_card == entt::null) {
            return;
        }
        m_ui.Destroy(m_card);
        m_ui.Destroy(m_from);
        m_ui.Destroy(m_to);
        m_card = m_pointer = m_label = m_from = m_to = entt::null;
    }

    //----------------------------------------------------------------------------
    //! 手本の次の1回を用意します。
    //----------------------------------------------------------------------------
    bool TutorialGuide::PrepareDemoCycle() {
        Tsukino::ECS::Registry&    registry = m_ui.Scene().GetRegistry();
        const Match::MatchSession& session  = Match::Session(registry);
        Board::BoardView&          board    = Board::View(registry);
        const auto&                hand     = session.match.Players()[0].hand;
        const size_t               count    = std::min(board.HandSize(0), hand.size());
        if(count == 0) {
            return false;
        }

        // 運ぶのは手札の真ん中のカード。行き先は正解を教えないよう、列の左端と右端の隙間を交互にする
        const size_t index = count / 2;
        const size_t slots = board.Row().size() + 1;
        const size_t slot  = (m_cycle % 2 == 0) ? slots - 1 : 0;

        auto& from          = registry.GetComponent<BuiltIn::WorldAnchorComponent>(m_from);
        from.target         = board.HandCard(0, index);
        auto& to            = registry.GetComponent<BuiltIn::WorldAnchorComponent>(m_to);
        to.useFixedWorldPosition = true;
        to.fixedWorldPosition    = board.SlotPosition(slot);
        // 列の端の隙間は列のカードの縁にあるので、カード半分ほど外へ出して「隙間に入れる」ように見せる
        to.worldOffset = hlslpp::float3((slot == 0 ? -0.6f : 0.6f) * Board::kCardWidth, 0.0f, 0.0f);

        m_ui.SetPicture(m_card, Util::AssetPaths::CardHiddenTexture(hand[index].id));
        m_time   = 0.0f;
        m_settle = 2;
        return true;
    }

}    // namespace ChronoSort::UI
