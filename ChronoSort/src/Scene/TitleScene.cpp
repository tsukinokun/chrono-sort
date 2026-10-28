//----------------------------------------------------------------------------
//! @file   TitleScene.cpp
//! @brief  タイトル画面の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Scene/TitleScene.hpp>

#include <ChronoSort/Game/FrameMonitor.hpp>
#include <ChronoSort/Game/SaveData.hpp>
#include <ChronoSort/Game/Sounds.hpp>
#include <ChronoSort/Scene/CreditsScene.hpp>
#include <ChronoSort/Scene/GameScene.hpp>
#include <ChronoSort/Util/TextUtil.hpp>

#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <algorithm>
#include <cmath>
#include <iterator>
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
        Tsukino::ECS::Entity title = entt::null;
        {
            UI::PaperSpec logo;
            logo.texture      = "title_logo.png";
            logo.center       = hlslpp::float2(760.0f, 205.0f);
            logo.size         = hlslpp::float2(720.0f, 208.0f);
            logo.angleDegrees = -2.5f;
            logo.swayDegrees  = 0.6f;
            logo.swaySeconds  = 5.0f;
            logo.sortOrder    = kLayerMenu;
            title             = m_ui->Paper(logo);
            m_ui->SlideIn(title, hlslpp::float2(0.0f, -320.0f), 0.3f, 0.8f);
        }

        // 出題の範囲（前回選んだものから始める）
        m_save  = Game::LoadSaveData();
        m_range = Rules::RangeFromKey(m_save.range);
        CreateRangeTabs(title);

        // メニュー（あえて高さも傾きも揃えない）
        m_solo    = CreateMenuNote(L"ひとりで挑む", L"インク三つで、何枚並べられるか", hlslpp::float2(470.0f, 500.0f), -5.0f, 0.7f, "pin.png");
        m_versus  = CreateMenuNote(L"CPUと対戦", L"先に手札を出し切った者の勝ち", hlslpp::float2(860.0f, 548.0f), 3.5f, 0.85f, "tape.png");
        m_credits = CreateMenuNote(L"出典・クレジット", L"", hlslpp::float2(1205.0f, 690.0f), -2.0f, 1.0f, "pin.png");
        m_quit    = CreateMenuNote(L"机を離れる", L"", hlslpp::float2(318.0f, 742.0f), 6.0f, 1.1f, "tape.png");

        // 最高記録のメモ（選んでいる範囲に記録があるときだけ出す）
        {
            UI::PaperSpec memo;
            memo.texture      = "scrap_square.png";
            memo.center       = hlslpp::float2(1300.0f, 330.0f);
            memo.size         = hlslpp::float2(240.0f, 250.0f);
            memo.angleDegrees = 7.0f;
            memo.sortOrder    = kLayerMenu;
            m_record          = m_ui->Paper(memo);
            m_recordTitle     = m_ui->Text(L"", hlslpp::float2(0.0f, -62.0f), 0.62f, UI::kSealRedColor, kLayerMenu + 1, m_record);
            const float rows[] = {-18.0f, 16.0f, 50.0f, 84.0f};
            for(size_t i = 0; i < std::size(rows); ++i) {
                const bool versus = i == std::size(rows) - 1;
                m_recordLines.push_back(m_ui->Text(L"", hlslpp::float2(-84.0f, rows[i]), versus ? 0.5f : 0.55f, versus ? UI::kFadedInkColor : UI::kInkColor,
                                                   kLayerMenu + 1, m_record, false));
            }
            // 最初は画面の外に置いておき、記録があれば滑り込ませる
            m_ui->SlideOut(m_record, hlslpp::float2(320.0f, 0.0f), 0.0f, 0.01f);
            RefreshRecordMemo(1.2f);
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
        UpdateRangeTabs(deltaTime);

        // カーソルが新しく紙に乗ったら、紙が擦れる小さな音
        std::vector<Tsukino::ECS::Entity> hovered;
        std::vector<Tsukino::ECS::Entity> candidates = {m_solo, m_versus, m_credits, m_quit, m_start, m_cancel};
        candidates.insert(candidates.end(), m_rangeTags.begin(), m_rangeTags.end());
        for(Tsukino::ECS::Entity e : candidates) {
            if(e != entt::null && m_ui->Hovered(e)) {
                hovered.push_back(e);
                if(std::find(m_hovered.begin(), m_hovered.end(), e) == m_hovered.end()) {
                    Game::FrameMonitor::Mark("hover menu");
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
                config.range = m_range;
                sounds.Play("button");
                api.ChangeScene(std::make_unique<GameScene>(config));
                return;
            }
            if(m_ui->Clicked(m_cancel) || context->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Escape)) {
                sounds.Play("page", 0.7f);
                CloseVersusSheet();
            }
        } else {
            for(size_t i = 0; i < m_rangeTags.size(); ++i) {
                if(m_ui->Clicked(m_rangeTags[i]) && m_rangeOrder[i] != m_range) {
                    sounds.Play("stamp", 0.6f);
                    SelectRange(m_rangeOrder[i]);
                }
            }
            if(m_ui->Clicked(m_solo)) {
                Rules::MatchConfig config;
                config.range = m_range;
                sounds.Play("button");
                api.ChangeScene(std::make_unique<GameScene>(config));
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
        // 何で遊ぶかを始める前に確かめられるよう、選んでいる範囲を添える
        m_ui->Text(L"出題：" + Util::Utf8ToWide(Rules::RangeName(m_range)), hlslpp::float2(0.0f, -136.0f), 0.5f, UI::kFadedInkColor, layer, m_sheet);
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

    //----------------------------------------------------------------------------
    //! 出題の範囲のしおりを3枚作ります。
    //----------------------------------------------------------------------------
    void TitleScene::CreateRangeTabs(Tsukino::ECS::Entity title) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();

        // 題字の左下に小さな見出し（題字と一緒に揺れる）
        m_ui->Text(L"出題", hlslpp::float2(-392.0f, 150.0f), 0.5f, UI::kParchmentColor, kLayerMenu + 1, title);

        // 高さも傾きもわざと揃えない
        struct TabSpec {
            Rules::CardRange range;
            hlslpp::float2   center;
            float            angle;
        };
        const TabSpec specs[] = {
            {Rules::CardRange::Japan, hlslpp::float2(510.0f, 360.0f), -3.0f},
            {Rules::CardRange::World, hlslpp::float2(683.0f, 352.0f), 2.5f},
            {Rules::CardRange::All, hlslpp::float2(856.0f, 364.0f), -1.5f},
        };
        for(size_t i = 0; i < std::size(specs); ++i) {
            const TabSpec& spec = specs[i];

            // 持ち上げは親を動かして行う（紙片の揺れ・ボタンの反応は紙片側に残す）
            const Tsukino::ECS::Entity holder = m_scene.CreateEntity();
            registry.AddComponent<BuiltIn::TransformComponent>(holder).position = hlslpp::float3(spec.center, 0.0f);

            UI::PaperSpec paper;
            paper.texture      = "scrap_note.png";
            paper.size         = hlslpp::float2(164.0f, 70.0f);
            paper.angleDegrees = spec.angle;
            paper.swayDegrees  = 0.7f;
            paper.sortOrder    = kLayerMenu;
            paper.parent       = holder;
            const auto tag     = m_ui->Paper(paper);

            const bool all = spec.range == Rules::CardRange::All;
            m_rangeTexts.push_back(m_ui->Text(Util::Utf8ToWide(Rules::RangeName(spec.range)), hlslpp::float2(4.0f, all ? -9.0f : 2.0f), 0.78f, UI::kInkColor,
                                              kLayerMenu + 1, tag));
            if(all) {
                m_ui->Text(L"日本と世界", hlslpp::float2(4.0f, 20.0f), 0.42f, UI::kFadedInkColor, kLayerMenu + 1, tag);
            }
            m_ui->MakeButton(tag);
            m_ui->SlideIn(tag, hlslpp::float2(m_ui->Jitter() * 60.0f, -260.0f), 0.45f + 0.07f * static_cast<float>(i), 0.6f);

            m_rangeOrder.push_back(spec.range);
            m_rangeHolders.push_back(holder);
            m_rangeTags.push_back(tag);
            m_rangeBases.push_back(spec.center);
            m_rangeLift.push_back(spec.range == m_range ? 1.0f : 0.0f);
        }

        // 選んでいるしおりに垂らす紐（紙片の後ろから下へ垂れる）
        UI::PaperSpec ribbon;
        ribbon.texture     = "ribbon.png";
        ribbon.size        = hlslpp::float2(24.0f, 84.0f);
        ribbon.swayDegrees = 3.0f;
        ribbon.swaySeconds = 2.4f;
        ribbon.sortOrder   = kLayerMenu - 1;
        m_rangeRibbon      = m_ui->Paper(ribbon);
        SelectRange(m_range);
    }

    //----------------------------------------------------------------------------
    //! しおりの持ち上がりを、選んでいる範囲へなめらかに寄せます。
    //----------------------------------------------------------------------------
    void TitleScene::UpdateRangeTabs(float deltaTime) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        const float             rate     = 1.0f - std::exp(-12.0f * deltaTime);
        for(size_t i = 0; i < m_rangeHolders.size(); ++i) {
            const float target = m_rangeOrder[i] == m_range ? 1.0f : 0.0f;
            m_rangeLift[i] += (target - m_rangeLift[i]) * rate;

            // 選んだ札は持ち上げて大きく、ほかは少し沈めて小さく
            const float lift      = m_rangeLift[i];
            auto&       transform = registry.GetComponent<BuiltIn::TransformComponent>(m_rangeHolders[i]);
            transform.position    = hlslpp::float3(m_rangeBases[i] + hlslpp::float2(0.0f, 5.0f - 21.0f * lift), 0.0f);
            const float scale     = 0.93f + 0.14f * lift;
            transform.scale       = hlslpp::float3(scale, scale, 1.0f);
            transform.dirty       = true;
        }
    }

    //----------------------------------------------------------------------------
    //! 出題の範囲を選び直します。
    //----------------------------------------------------------------------------
    void TitleScene::SelectRange(Rules::CardRange range) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        const bool              changed  = range != m_range;
        m_range                          = range;

        for(size_t i = 0; i < m_rangeTags.size(); ++i) {
            const bool chosen = m_rangeOrder[i] == range;
            registry.GetComponent<BuiltIn::FontComponent>(m_rangeTexts[i]).color = chosen ? UI::kInkColor : UI::kFadedInkColor;
            if(chosen && m_rangeRibbon != entt::null) {
                // 紐を選んだしおりへ付け替え、上から垂らし直す
                auto& ribbon    = registry.GetComponent<BuiltIn::TransformComponent>(m_rangeRibbon);
                ribbon.parent   = m_rangeHolders[i];
                ribbon.position = hlslpp::float3(56.0f, 46.0f, 0.0f);
                ribbon.dirty    = true;
                if(changed) {
                    m_ui->SlideIn(m_rangeRibbon, hlslpp::float2(0.0f, -50.0f), 0.0f, 0.4f);
                    m_ui->Punch(m_rangeTags[i], 0.18f);
                }
            }
        }

        if(changed) {
            // 次に起動したときも同じ範囲から始められるよう、すぐ覚えておく
            m_save.range = Rules::RangeKey(range);
            Game::StoreSaveData(m_save);
            RefreshRecordMemo();
        }
    }

    //----------------------------------------------------------------------------
    //! 記録のメモを、選んでいる範囲の記録に合わせます。
    //----------------------------------------------------------------------------
    void TitleScene::RefreshRecordMemo(float slideDelay) {
        if(m_record == entt::null) {
            return;
        }
        const Game::SoloRecord& record = m_save.Record(m_range);
        const bool              show   = record.HasAny() || m_save.versusWins > 0;

        m_ui->SetText(m_recordTitle, Util::Utf8ToWide(Rules::RangeName(m_range)) + L"の記録");
        m_ui->SetText(m_recordLines[0], L"最高得点  " + std::to_wstring(record.bestScore));
        m_ui->SetText(m_recordLines[1], L"最多枚数  " + std::to_wstring(record.bestPlaced));
        m_ui->SetText(m_recordLines[2], L"最長連続  " + std::to_wstring(record.bestStreak));
        m_ui->SetText(m_recordLines[3], L"対戦の勝利  " + std::to_wstring(m_save.versusWins));

        if(show && !m_recordShown) {
            m_ui->SlideIn(m_record, hlslpp::float2(320.0f, 0.0f), slideDelay);
        } else if(!show && m_recordShown) {
            m_ui->SlideOut(m_record, hlslpp::float2(320.0f, 0.0f));
        } else if(show) {
            // 中身が変わったことが分かるよう、紙を軽く揺らす
            m_ui->Punch(m_record, 0.08f);
        }
        m_recordShown = show;
    }

}    // namespace ChronoSort::Scene
