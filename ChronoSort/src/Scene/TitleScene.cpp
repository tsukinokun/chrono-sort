//----------------------------------------------------------------------------
//! @file   TitleScene.cpp
//! @brief  タイトル画面の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Scene/TitleScene.hpp>

#include <ChronoSort/ECS/Component/ChoiceTagComponent.hpp>
#include <ChronoSort/Game/GameServices.hpp>
#include <ChronoSort/Game/SaveData.hpp>
#include <ChronoSort/Game/TuningSet.hpp>
#include <ChronoSort/Scene/SceneRouter.hpp>
#include <ChronoSort/UI/Widgets.hpp>
#include <ChronoSort/Util/AssetPaths.hpp>
#include <ChronoSort/Util/Localization.hpp>
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

        const char* const kLevelKeys[] = {"cpuLevel.easy", "cpuLevel.normal", "cpuLevel.hard"};    // CPU の強さの表示名のキー
    }    // namespace

    //----------------------------------------------------------------------------
    //! シーンを初期化します。
    //----------------------------------------------------------------------------
    void TitleScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI&) {
        auto* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        m_stage.Build(m_scene, Game::Tuning().Stage().titleCards);
        m_ui     = std::make_unique<UI::PaperUI>(m_scene, *context->assetManager);
        m_layout = UI::PaperLayout(Util::AssetPaths::UILayout("Title"));

        // 四隅を暗くしてランプの明かりに見せる
        {
            const auto vignette = m_ui->Paper(m_layout.Spec("vignette"));
            m_scene.GetRegistry().GetComponent<BuiltIn::SpriteComponent>(vignette).raycastTarget = false;
        }

        // 題字
        const Tsukino::ECS::Entity title = m_ui->Paper(m_layout.Spec("logo"));
        m_ui->SlideIn(title, hlslpp::float2(0.0f, -320.0f), 0.3f, 0.8f);

        // 出題の範囲と難易度（前回選んだものから始める）
        m_range      = Rules::RangeFromKey(Game::Services().Save().range);
        m_difficulty = Rules::DifficultyFromKey(Game::Services().Save().difficulty);
        CreateProblemTabs(title);

        // メニュー（あえて高さも傾きも揃えない）
        m_solo     = CreateMenuNote(Util::Text("title.solo"), Util::Text("title.soloNote"), "solo", 0.7f, "pin.png");
        m_versus   = CreateMenuNote(Util::Text("title.versus"), Util::Text("title.versusNote"), "versus", 0.85f, "tape.png");
        m_credits  = CreateMenuNote(Util::Text("title.credits"), L"", "credits", 1.0f, "pin.png");
        m_quit     = CreateMenuNote(Util::Text("title.quit"), L"", "quit", 1.1f, "tape.png");
        m_settings = CreateMenuNote(Util::Text("title.options"), L"", "settings", 1.05f, "pin.png");
        m_options  = std::make_unique<UI::OptionsSheet>(m_scene, *context, kLayerSheet + 20, true);

        // 最高記録のメモ（選んでいる範囲に記録があるときだけ出す）
        {
            m_record      = m_ui->Paper(m_layout.Spec("recordMemo"));
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

        Game::Services().Sounds().PlayBgm("title");
    }

    //----------------------------------------------------------------------------
    //! メニューのメモ紙を1枚作ります。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity TitleScene::CreateMenuNote(const std::wstring& label, const std::wstring& note, const std::string& layout, float delay,
                                                    const char* fastener) {
        const bool          hasNote = !note.empty();
        const UI::PaperSpec spec    = m_layout.Spec(layout);
        const auto          paper   = m_ui->Paper(spec);
        m_ui->Text(label, hlslpp::float2(0.0f, hasNote ? -16.0f : 2.0f), hasNote ? 1.0f : 0.8f, UI::kInkColor, kLayerMenu + 1, paper);
        if(hasNote) {
            m_ui->Text(note, hlslpp::float2(0.0f, 26.0f), 0.46f, UI::kFadedInkColor, kLayerMenu + 1, paper);
        }

        // 留め具：ピンは左上、テープは右上に斜めに貼る（Title.json の center は紙の角からのずれ）
        const bool    pin    = std::string(fastener) == "pin.png";
        UI::PaperSpec fix    = m_layout.Spec(pin ? "menuPin" : "menuTape");
        const float   halfW  = float(spec.size.x) * 0.5f;
        const float   halfH  = float(spec.size.y) * 0.5f;
        fix.center           = hlslpp::float2(pin ? -halfW + float(fix.center.x) : halfW + float(fix.center.x), -halfH + float(fix.center.y));
        fix.sortOrder        = kLayerMenu + 2;
        fix.parent           = paper;
        m_ui->Paper(fix);

        m_ui->MakeButton(paper);
        m_ui->SlideIn(paper, hlslpp::float2(m_ui->Jitter() * 120.0f, 380.0f), delay, 0.6f);
        return paper;
    }

    //----------------------------------------------------------------------------
    //! シーンを更新します。
    //----------------------------------------------------------------------------
    void TitleScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        auto*                   context  = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        Game::SoundBank&        sounds   = Game::Services().Sounds();
        m_stage.Update(deltaTime);
        // ボタンに重なったときの音は、エンジンの UIButtonSoundSystem が鳴らす（PaperUI::MakeButton で付けた音）

        // 設定の紙が開いている間（閉じたフレームも含む）は、下のメニューを受け付けない
        // （閉じた Esc で、続けて「机を離れる」まで動かないように）
        const bool optionsWereOpen = m_options->IsOpen();
        const bool optionsClosed   = m_options->Update(deltaTime);
        // 言語を選び直したら、文字列の表を読み直し、その言語のカードの面を読込画面で読んでから、タイトルを作り直す
        if(optionsClosed && m_options->LanguageChanged()) {
            Util::LoadLanguage(Util::LanguageFromCode(Game::Services().Settings().language));
            Router::ReloadTitle(api);
            m_scene.Update(deltaTime);
            return;
        }
        if(optionsClosed || optionsWereOpen) {
            m_scene.Update(deltaTime);
            return;
        }

        if(m_sheet != entt::null) {
            // 札の見た目と選んだ音は ChoiceTagSystem。ここでは選ばれた値を受け取るだけ
            m_cpuCount = UI::SelectedChoice(registry, m_countGroup, m_cpuCount);
            m_cpuLevel = static_cast<Rules::CpuLevel>(UI::SelectedChoice(registry, m_levelGroup, static_cast<int>(m_cpuLevel)));
            if(m_ui->Clicked(m_start) || context->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Enter)) {
                sounds.Play("ui.button");
                Router::ToMatch(api, Router::Versus(m_range, m_difficulty, std::vector<Rules::CpuLevel>(static_cast<size_t>(m_cpuCount), m_cpuLevel)));
                return;
            }
            if(m_ui->Clicked(m_cancel) || context->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Escape)) {
                sounds.Play("ui.close");
                CloseVersusSheet();
            }
        } else {
            if(UI::ChoiceChanged(registry, m_rangeRow.group)) {
                m_range = static_cast<Rules::CardRange>(UI::SelectedChoice(registry, m_rangeRow.group, static_cast<int>(m_range)));
                HangRibbon(m_rangeRow, static_cast<int>(m_range), true);
                OnProblemSetChanged();
            }
            if(UI::ChoiceChanged(registry, m_difficultyRow.group)) {
                m_difficulty = static_cast<Rules::CardDifficulty>(UI::SelectedChoice(registry, m_difficultyRow.group, static_cast<int>(m_difficulty)));
                HangRibbon(m_difficultyRow, static_cast<int>(m_difficulty), true);
                OnProblemSetChanged();
            }
            if(m_ui->Clicked(m_solo)) {
                sounds.Play("ui.button");
                Router::ToMatch(api, Router::Solo(m_range, m_difficulty));
                return;
            }
            if(m_ui->Clicked(m_versus)) {
                sounds.Play("ui.open");
                OpenVersusSheet();
            }
            if(m_ui->Clicked(m_credits)) {
                sounds.Play("ui.open");
                Router::ToCredits(api);
                return;
            }
            if(m_ui->Clicked(m_settings)) {
                sounds.Play("ui.open");
                m_options->Open();
                m_scene.Update(deltaTime);
                return;
            }
            if(m_ui->Clicked(m_quit) || context->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Escape)) {
                Router::Quit();
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
        UI::PaperSpec sheet = m_layout.Spec("versusSheet");
        sheet.sortOrder     = kLayerSheet;
        m_sheet             = m_ui->Paper(sheet);
        m_ui->SlideIn(m_sheet, hlslpp::float2(0.0f, 600.0f), 0.0f, 0.5f);

        const int layer = kLayerSheet + 1;
        m_ui->Text(Util::Text("versus.title"), hlslpp::float2(0.0f, -175.0f), 1.15f, UI::kInkColor, layer, m_sheet);
        // 何で遊ぶかを始める前に確かめられるよう、選んでいる範囲を添える
        m_ui->Text(Util::Format("versus.problemSet", {Rules::ProblemSetName(m_range, m_difficulty)}),hlslpp::float2(0.0f, -136.0f), 0.5f, UI::kFadedInkColor, layer, m_sheet);
        m_ui->Text(Util::Text("versus.count"), hlslpp::float2(-200.0f, -105.0f), 0.62f, UI::kFadedInkColor, layer, m_sheet, false);
        m_ui->Text(Util::Text("versus.level"), hlslpp::float2(-200.0f, 10.0f), 0.62f, UI::kFadedInkColor, layer, m_sheet, false);

        // 札は少しずつ傾けて、手で置いたように並べる（開いたときは今の選択を弾ませて見せる）
        auto tag = [&](Tsukino::ECS::Entity group, const std::wstring& text, float x, float y, int value, bool selected) {
            UI::ChoiceSpec spec;
            spec.paper              = m_layout.Spec("versusTag");
            spec.paper.center       = hlslpp::float2(x, y + m_ui->Jitter() * 6.0f);
            spec.paper.angleDegrees = m_ui->Jitter() * 5.0f;
            spec.paper.sortOrder    = layer + 1;
            spec.paper.parent       = m_sheet;
            spec.label              = text;
            spec.value              = value;
            spec.punchOnCreate      = true;
            UI::ChoiceTag(*m_ui, group, spec, selected);
        };
        m_countGroup = UI::ChoiceGroup(*m_ui);
        m_levelGroup = UI::ChoiceGroup(*m_ui);
        for(int i = 0; i < 3; ++i) {
            tag(m_countGroup, Util::Format("versus.people", {std::to_wstring(i + 1)}), -150.0f + 150.0f * static_cast<float>(i), -55.0f, i + 1, i + 1 == m_cpuCount);
        }
        for(int i = 0; i < 3; ++i) {
            tag(m_levelGroup, Util::Text(kLevelKeys[i]), -150.0f + 150.0f * static_cast<float>(i), 60.0f, i, i == static_cast<int>(m_cpuLevel));
        }

        auto button = [&](const char* name, const std::wstring& text) {
            UI::PaperSpec spec = m_layout.Spec(name);
            spec.sortOrder     = layer + 1;
            spec.parent        = m_sheet;
            return UI::NoteButton(*m_ui, spec, text, 0.75f);
        };
        m_start  = button("versusStart", Util::Text("versus.start"));
        m_cancel = button("versusCancel", Util::Text("versus.cancel"));
    }

    //----------------------------------------------------------------------------
    //! CPU 対戦の設定の紙を閉じます。
    //----------------------------------------------------------------------------
    void TitleScene::CloseVersusSheet() {
        // 札の組も一緒に片付ける（組は紙の子ではないので別に破棄する）
        m_ui->Destroy(m_sheet);
        m_ui->Destroy(m_countGroup);
        m_ui->Destroy(m_levelGroup);
        m_sheet      = entt::null;
        m_start      = entt::null;
        m_cancel     = entt::null;
        m_countGroup = entt::null;
        m_levelGroup = entt::null;
    }

    //----------------------------------------------------------------------------
    //! 出題の範囲と難易度のしおりを作ります。
    //----------------------------------------------------------------------------
    void TitleScene::CreateProblemTabs(Tsukino::ECS::Entity title) {
        // 題字の左下に小さな見出し（題字と一緒に揺れる）
        m_ui->Text(Util::Text("title.rangeHeading"), hlslpp::float2(-392.0f, 150.0f), 0.5f, UI::kParchmentColor, kLayerMenu + 1, title, true, false);
        m_ui->Text(Util::Text("title.difficultyHeading"), hlslpp::float2(-400.0f, 266.0f), 0.5f, UI::kParchmentColor, kLayerMenu + 1, title, true, false);

        // 置き場所・傾きは Data/UI/Title.json（高さも傾きもわざと揃えない）
        using Rules::CardDifficulty;
        using Rules::CardRange;
        auto range = [](CardRange r, const char* layout, const std::wstring& note) {
            return TabSpec{static_cast<int>(r), layout, Rules::RangeName(r), note};
        };
        auto difficulty = [](CardDifficulty d, const char* layout) {
            return TabSpec{static_cast<int>(d), layout, Rules::DifficultyName(d), L""};
        };
        CreateTabRow(m_rangeRow, {range(CardRange::Japan, "rangeJapan", L""), range(CardRange::World, "rangeWorld", L""), range(CardRange::All, "rangeAll", Util::Text("title.rangeAllNote"))},
                     static_cast<int>(m_range), 0.78f, "rangeRibbon", 0.45f);
        CreateTabRow(m_difficultyRow,
                     {difficulty(CardDifficulty::Easy, "difficultyEasy"), difficulty(CardDifficulty::Normal, "difficultyNormal"),
                      difficulty(CardDifficulty::Hard, "difficultyHard")},
                     static_cast<int>(m_difficulty), 0.62f, "difficultyRibbon", 0.6f);
    }

    //----------------------------------------------------------------------------
    //! しおりの段を作ります。
    //----------------------------------------------------------------------------
    void TitleScene::CreateTabRow(TabRow& row, const std::vector<TabSpec>& tabs, int selected, float labelScale, const char* ribbonLayout, float delay) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        row.group        = UI::ChoiceGroup(*m_ui);
        row.ribbonLayout = ribbonLayout;
        for(size_t i = 0; i < tabs.size(); ++i) {
            const TabSpec& tab     = tabs[i];
            const bool     hasNote = !tab.note.empty();

            // 持ち上げは親を動かして行う（紙片の揺れ・ボタンの反応は紙片側に残す）。紙片は親の真ん中に置く
            UI::ChoiceSpec spec;
            spec.paper              = m_layout.Spec(tab.layout);
            const hlslpp::float2 at = spec.paper.center;
            const Tsukino::ECS::Entity holder = m_scene.CreateEntity();
            registry.AddComponent<BuiltIn::TransformComponent>(holder).position = hlslpp::float3(at, 0.0f);
            spec.paper.center = hlslpp::float2(0.0f, 0.0f);
            spec.paper.parent = holder;
            spec.label        = tab.label;
            spec.labelScale   = labelScale;
            spec.labelOffset  = hlslpp::float2(4.0f, hasNote ? -9.0f : 2.0f);
            spec.value        = tab.value;
            spec.chosenColor  = UI::kInkColor;
            spec.punch        = 0.18f;
            const auto tag    = UI::ChoiceTag(*m_ui, row.group, spec, tab.value == selected);
            if(hasNote) {
                m_ui->Text(tab.note, hlslpp::float2(4.0f, 13.0f), 0.42f, UI::kFadedInkColor, kLayerMenu + 1, tag);
            }
            m_ui->SlideIn(tag, hlslpp::float2(m_ui->Jitter() * 60.0f, -260.0f), delay + 0.07f * static_cast<float>(i), 0.6f);

            auto& lift = registry.AddComponent<ECS::ChoiceLiftComponent>(holder);
            lift.tag   = tag;
            lift.base  = at;
            lift.lift  = tab.value == selected ? 1.0f : 0.0f;
            row.holders.emplace_back(tab.value, holder);
        }

        // 選んでいるしおりに垂らす紐（紙片の後ろから下へ垂れる）
        row.ribbon = m_ui->Paper(m_layout.Spec(ribbonLayout));
        HangRibbon(row, selected, false);
    }

    //----------------------------------------------------------------------------
    //! 段の紐を、選んでいるしおりへ付け替えます。
    //----------------------------------------------------------------------------
    void TitleScene::HangRibbon(TabRow& row, int value, bool changed) {
        for(const auto& [tabValue, holder] : row.holders) {
            if(tabValue != value || row.ribbon == entt::null) {
                continue;
            }
            auto& ribbon    = m_scene.GetRegistry().GetComponent<BuiltIn::TransformComponent>(row.ribbon);
            ribbon.parent   = holder;
            ribbon.position = hlslpp::float3(m_layout.Spec(row.ribbonLayout).center, 0.0f);
            ribbon.dirty    = true;
            if(changed) {
                m_ui->SlideIn(row.ribbon, hlslpp::float2(0.0f, -50.0f), 0.0f, 0.4f);
            }
        }
    }

    //----------------------------------------------------------------------------
    //! 出題の範囲か難易度を選び直したときに、記録のメモと保存を更新します。
    //----------------------------------------------------------------------------
    void TitleScene::OnProblemSetChanged() {
        // 次に起動したときも同じ出題から始められるよう、すぐ覚えておく
        Game::Services().Save().range      = Rules::RangeKey(m_range);
        Game::Services().Save().difficulty = Rules::DifficultyKey(m_difficulty);
        Game::Services().StoreSave();
        RefreshRecordMemo();
    }

    //----------------------------------------------------------------------------
    //! 記録のメモを、選んでいる範囲の記録に合わせます。
    //----------------------------------------------------------------------------
    void TitleScene::RefreshRecordMemo(float slideDelay) {
        if(m_record == entt::null) {
            return;
        }
        const Game::SaveData&   save   = Game::Services().Save();
        const Game::SoloRecord& record = save.Record(m_range, m_difficulty);
        const bool              show   = record.HasAny() || save.versusWins > 0;

        m_ui->SetText(m_recordTitle, Util::Format("title.record", {Rules::ProblemSetName(m_range, m_difficulty)}));
        m_ui->SetText(m_recordLines[0], Util::Format("title.bestScore", {std::to_wstring(record.bestScore)}));
        m_ui->SetText(m_recordLines[1], Util::Format("title.bestPlaced", {std::to_wstring(record.bestPlaced)}));
        m_ui->SetText(m_recordLines[2], Util::Format("title.bestStreak", {std::to_wstring(record.bestStreak)}));
        m_ui->SetText(m_recordLines[3], Util::Format("title.versusWins", {std::to_wstring(save.versusWins)}));

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
