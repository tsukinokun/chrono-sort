//----------------------------------------------------------------------------
//! @file   TitleScene.cpp
//! @brief  タイトル画面の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Scene/TitleScene.hpp>

#include <ChronoSort/Game/SaveData.hpp>
#include <ChronoSort/Game/Sounds.hpp>
#include <ChronoSort/Scene/CreditsScene.hpp>
#include <ChronoSort/Scene/GameScene.hpp>

#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <algorithm>
#include <string>

namespace ChronoSort::Scene {

    namespace {
        namespace BuiltIn = Tsukino::BuiltIn::ECS;

        constexpr int kLayerMenu  = 20;
        constexpr int kLayerSheet = 70;

        const wchar_t* const kLevelNames[] = {L"やさしい", L"ふつう", L"てごわい"};
    }    // namespace

    //----------------------------------------------------------------------------
    //! シーンを初期化します。
    //----------------------------------------------------------------------------
    void TitleScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI&) {
        auto* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        m_stage.Build(m_scene, 14);
        m_ui = std::make_unique<UI::PaperUI>(m_scene, *context->assetManager);

        // 四隅を暗くしてランプの明かりに見せる
        {
            UI::PaperSpec spec;
            spec.texture     = "vignette.png";
            spec.center      = hlslpp::float2(UI::kReferenceWidth * 0.5f, UI::kReferenceHeight * 0.5f);
            spec.size        = hlslpp::float2(UI::kReferenceWidth, UI::kReferenceHeight);
            spec.swayDegrees = 0.0f;
            spec.bobPixels   = 0.0f;
            spec.sortOrder   = -50;
            const auto vignette = m_ui->Paper(spec);
            m_scene.GetRegistry().GetComponent<BuiltIn::SpriteComponent>(vignette).raycastTarget = false;
        }

        // 題字
        {
            UI::PaperSpec logo;
            logo.texture      = "title_logo.png";
            logo.center       = hlslpp::float2(760.0f, 205.0f);
            logo.size         = hlslpp::float2(720.0f, 208.0f);
            logo.angleDegrees = -2.5f;
            logo.swayDegrees  = 0.6f;
            logo.swaySeconds  = 5.0f;
            logo.sortOrder    = kLayerMenu;
            const auto title  = m_ui->Paper(logo);
            m_ui->SlideIn(title, hlslpp::float2(0.0f, -320.0f), 0.3f, 0.8f);
        }

        // メニュー（あえて高さも傾きも揃えない）
        m_solo    = CreateMenuNote(L"ひとりで挑む", L"インク三つで、何枚並べられるか", hlslpp::float2(470.0f, 500.0f), -5.0f, 0.7f, "pin.png");
        m_versus  = CreateMenuNote(L"CPUと対戦", L"Timeline のルールで、先に出し切った者の勝ち", hlslpp::float2(860.0f, 548.0f), 3.5f, 0.85f, "tape.png");
        m_credits = CreateMenuNote(L"出典・クレジット", L"", hlslpp::float2(1205.0f, 690.0f), -2.0f, 1.0f, "pin.png");
        m_quit    = CreateMenuNote(L"机を離れる", L"", hlslpp::float2(318.0f, 742.0f), 6.0f, 1.1f, "tape.png");

        // 最高記録のメモ（記録があるときだけ）
        const Game::SaveData save = Game::LoadSaveData();
        if(save.bestScore > 0 || save.versusWins > 0) {
            UI::PaperSpec memo;
            memo.texture      = "scrap_square.png";
            memo.center       = hlslpp::float2(1300.0f, 330.0f);
            memo.size         = hlslpp::float2(230.0f, 200.0f);
            memo.angleDegrees = 7.0f;
            memo.sortOrder    = kLayerMenu;
            const auto note   = m_ui->Paper(memo);
            m_ui->Text(L"これまでの記録", hlslpp::float2(0.0f, -62.0f), 0.62f, UI::kSealRedColor, kLayerMenu + 1, note);
            m_ui->Text(L"最高得点  " + std::to_wstring(save.bestScore), hlslpp::float2(-84.0f, -18.0f), 0.55f, UI::kInkColor, kLayerMenu + 1, note, false);
            m_ui->Text(L"最多枚数  " + std::to_wstring(save.bestPlaced), hlslpp::float2(-84.0f, 16.0f), 0.55f, UI::kInkColor, kLayerMenu + 1, note, false);
            m_ui->Text(L"最長連続  " + std::to_wstring(save.bestStreak), hlslpp::float2(-84.0f, 50.0f), 0.55f, UI::kInkColor, kLayerMenu + 1, note, false);
            m_ui->Text(L"対戦の勝利  " + std::to_wstring(save.versusWins), hlslpp::float2(-84.0f, 84.0f), 0.5f, UI::kFadedInkColor, kLayerMenu + 1, note, false);
            m_ui->SlideIn(note, hlslpp::float2(320.0f, 0.0f), 1.2f);
        }

        Game::Sounds(*context).PlayBgm("title");
    }

    //----------------------------------------------------------------------------
    //! メニューのメモ紙を1枚作ります。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity TitleScene::CreateMenuNote(const std::wstring& label, const std::wstring& note, const hlslpp::float2& center, float angle, float delay,
                                                    const char* fastener) {
        const bool    hasNote = !note.empty();
        UI::PaperSpec spec;
        spec.texture      = hasNote ? "scrap_wide.png" : "scrap_note.png";
        spec.center       = center;
        spec.size         = hasNote ? hlslpp::float2(400.0f, 118.0f) : hlslpp::float2(270.0f, 86.0f);
        spec.angleDegrees = angle;
        spec.swayDegrees  = 0.9f;
        spec.sortOrder    = kLayerMenu;
        const auto paper  = m_ui->Paper(spec);
        m_ui->Text(label, hlslpp::float2(0.0f, hasNote ? -16.0f : 2.0f), hasNote ? 1.0f : 0.8f, UI::kInkColor, kLayerMenu + 1, paper);
        if(hasNote) {
            const auto sub = m_ui->Text(note, hlslpp::float2(0.0f, 26.0f), 0.46f, UI::kFadedInkColor, kLayerMenu + 1, paper);
            m_scene.GetRegistry().GetComponent<BuiltIn::FontComponent>(sub).maxWidth = 360.0f;
        }

        // 留め具：ピンは左上、テープは右上に斜めに貼る
        const bool    pin = std::string(fastener) == "pin.png";
        UI::PaperSpec fix;
        fix.texture      = fastener;
        fix.center       = pin ? hlslpp::float2(-float(spec.size.x) * 0.5f + 26.0f, -float(spec.size.y) * 0.5f + 16.0f)
                               : hlslpp::float2(float(spec.size.x) * 0.5f - 30.0f, -float(spec.size.y) * 0.5f + 4.0f);
        fix.size         = pin ? hlslpp::float2(40.0f, 40.0f) : hlslpp::float2(110.0f, 34.0f);
        fix.angleDegrees = pin ? 0.0f : 32.0f;
        fix.swayDegrees  = 0.0f;
        fix.bobPixels    = 0.0f;
        fix.sortOrder    = kLayerMenu + 2;
        fix.parent       = paper;
        m_ui->Paper(fix);

        m_ui->MakeButton(paper);
        m_ui->SlideIn(paper, hlslpp::float2(m_ui->Jitter() * 120.0f, 380.0f), delay, 0.6f);
        return paper;
    }

    //----------------------------------------------------------------------------
    //! シーンを更新します。
    //----------------------------------------------------------------------------
    void TitleScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        auto*            context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        Game::SoundBank& sounds  = Game::Sounds(*context);
        m_stage.Update(deltaTime);

        // カーソルが新しく紙に乗ったら、紙が擦れる小さな音
        std::vector<Tsukino::ECS::Entity> hovered;
        for(Tsukino::ECS::Entity e : {m_solo, m_versus, m_credits, m_quit, m_start, m_cancel}) {
            if(e != entt::null && m_ui->Hovered(e)) {
                hovered.push_back(e);
                if(std::find(m_hovered.begin(), m_hovered.end(), e) == m_hovered.end()) {
                    sounds.Play("card_slide", 0.3f);
                }
            }
        }
        m_hovered = hovered;

        if(m_sheet != entt::null) {
            for(size_t i = 0; i < m_countTags.size(); ++i) {
                if(m_ui->Clicked(m_countTags[i])) {
                    m_cpuCount = static_cast<int>(i) + 1;
                    sounds.Play("stamp", 0.6f);
                    RefreshVersusChoices();
                }
            }
            for(size_t i = 0; i < m_levelTags.size(); ++i) {
                if(m_ui->Clicked(m_levelTags[i])) {
                    m_cpuLevel = static_cast<Rules::CpuLevel>(i);
                    sounds.Play("stamp", 0.6f);
                    RefreshVersusChoices();
                }
            }
            if(m_ui->Clicked(m_start) || context->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Enter)) {
                Rules::MatchConfig config;
                config.mode = Rules::MatchMode::Versus;
                config.cpus.assign(static_cast<size_t>(m_cpuCount), m_cpuLevel);
                sounds.Play("button");
                api.ChangeScene(std::make_unique<GameScene>(config));
                return;
            }
            if(m_ui->Clicked(m_cancel) || context->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Escape)) {
                sounds.Play("page", 0.7f);
                CloseVersusSheet();
            }
        } else {
            if(m_ui->Clicked(m_solo)) {
                sounds.Play("button");
                api.ChangeScene(std::make_unique<GameScene>(Rules::MatchConfig{}));
                return;
            }
            if(m_ui->Clicked(m_versus)) {
                sounds.Play("page", 0.8f);
                OpenVersusSheet();
            }
            if(m_ui->Clicked(m_credits)) {
                sounds.Play("page", 0.8f);
                api.ChangeScene(std::make_unique<CreditsScene>());
                return;
            }
            if(m_ui->Clicked(m_quit) || context->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Escape)) {
                // メインループは WM_QUIT を受けると抜ける
                PostQuitMessage(0);
            }
        }

        m_scene.Update(deltaTime);
    }

    //----------------------------------------------------------------------------
    //! シーンの終了処理を行います。
    //----------------------------------------------------------------------------
    void TitleScene::OnExit() {
    }

    //----------------------------------------------------------------------------
    //! CPU 対戦の設定の紙を開きます。
    //----------------------------------------------------------------------------
    void TitleScene::OpenVersusSheet() {
        if(m_sheet != entt::null) {
            return;
        }
        UI::PaperSpec sheet;
        sheet.texture      = "scrap_result.png";
        sheet.center       = hlslpp::float2(840.0f, 470.0f);
        sheet.size         = hlslpp::float2(560.0f, 470.0f);
        sheet.angleDegrees = -2.0f;
        sheet.swayDegrees  = 0.4f;
        sheet.sortOrder    = kLayerSheet;
        m_sheet            = m_ui->Paper(sheet);
        m_ui->SlideIn(m_sheet, hlslpp::float2(0.0f, 600.0f), 0.0f, 0.5f);

        const int layer = kLayerSheet + 1;
        m_ui->Text(L"対戦の支度", hlslpp::float2(0.0f, -175.0f), 1.15f, UI::kInkColor, layer, m_sheet);
        m_ui->Text(L"相手の人数", hlslpp::float2(-200.0f, -105.0f), 0.62f, UI::kFadedInkColor, layer, m_sheet, false);
        m_ui->Text(L"相手の強さ", hlslpp::float2(-200.0f, 10.0f), 0.62f, UI::kFadedInkColor, layer, m_sheet, false);

        // 札は少しずつ傾けて、手で置いたように並べる
        auto tag = [&](const std::wstring& text, const hlslpp::float2& at, std::vector<Tsukino::ECS::Entity>& tags, std::vector<Tsukino::ECS::Entity>& texts) {
            UI::PaperSpec spec;
            spec.texture      = "scrap_note.png";
            spec.center       = at;
            spec.size         = hlslpp::float2(130.0f, 56.0f);
            spec.angleDegrees = m_ui->Jitter() * 5.0f;
            spec.swayDegrees  = 0.6f;
            spec.sortOrder    = layer + 1;
            spec.parent       = m_sheet;
            const auto paper  = m_ui->Paper(spec);
            texts.push_back(m_ui->Text(text, hlslpp::float2(4.0f, 2.0f), 0.6f, UI::kInkColor, layer + 2, paper));
            m_ui->MakeButton(paper);
            tags.push_back(paper);
        };
        for(int i = 0; i < 3; ++i) {
            tag(std::to_wstring(i + 1) + L" 人", hlslpp::float2(-150.0f + 150.0f * static_cast<float>(i), -55.0f + m_ui->Jitter() * 6.0f), m_countTags, m_countTexts);
        }
        for(int i = 0; i < 3; ++i) {
            tag(kLevelNames[i], hlslpp::float2(-150.0f + 150.0f * static_cast<float>(i), 60.0f + m_ui->Jitter() * 6.0f), m_levelTags, m_levelTexts);
        }

        auto button = [&](const std::wstring& text, const hlslpp::float2& at, float angle) {
            UI::PaperSpec spec;
            spec.texture      = "scrap_note.png";
            spec.center       = at;
            spec.size         = hlslpp::float2(190.0f, 66.0f);
            spec.angleDegrees = angle;
            spec.sortOrder    = layer + 1;
            spec.parent       = m_sheet;
            const auto paper  = m_ui->Paper(spec);
            m_ui->Text(text, hlslpp::float2(6.0f, 2.0f), 0.75f, UI::kInkColor, layer + 2, paper);
            m_ui->MakeButton(paper);
            return paper;
        };
        m_start  = button(L"始める", hlslpp::float2(95.0f, 165.0f), 3.0f);
        m_cancel = button(L"やめる", hlslpp::float2(-130.0f, 172.0f), -4.0f);
        RefreshVersusChoices();
    }

    //----------------------------------------------------------------------------
    //! CPU 対戦の設定の紙を閉じます。
    //----------------------------------------------------------------------------
    void TitleScene::CloseVersusSheet() {
        m_ui->Destroy(m_sheet);
        m_sheet  = entt::null;
        m_start  = entt::null;
        m_cancel = entt::null;
        m_countTags.clear();
        m_levelTags.clear();
        m_countTexts.clear();
        m_levelTexts.clear();
    }

    //----------------------------------------------------------------------------
    //! 設定の紙の選択肢の見た目を、今の選択に合わせます。
    //----------------------------------------------------------------------------
    void TitleScene::RefreshVersusChoices() {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        auto mark = [&](std::vector<Tsukino::ECS::Entity>& texts, std::vector<Tsukino::ECS::Entity>& tags, int selected) {
            for(size_t i = 0; i < texts.size(); ++i) {
                const bool chosen = static_cast<int>(i) == selected;
                // 選んだ札は朱で書き、少し弾ませる
                registry.GetComponent<BuiltIn::FontComponent>(texts[i]).color = chosen ? UI::kSealRedColor : UI::kFadedInkColor;
                if(chosen) {
                    m_ui->Punch(tags[i], 0.2f);
                }
            }
        };
        mark(m_countTexts, m_countTags, m_cpuCount - 1);
        mark(m_levelTexts, m_levelTags, static_cast<int>(m_cpuLevel));
    }

}    // namespace ChronoSort::Scene
