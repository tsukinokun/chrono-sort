//----------------------------------------------------------------------------
//! @file   CreditsScene.cpp
//! @brief  出典・クレジット画面の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Scene/CreditsScene.hpp>

#include <ChronoSort/ECS/Component/PaperComponent.hpp>
#include <ChronoSort/Game/GameServices.hpp>
#include <ChronoSort/Game/TuningSet.hpp>
#include <ChronoSort/Scene/SceneRouter.hpp>
#include <ChronoSort/UI/PaperLayout.hpp>
#include <ChronoSort/UI/Widgets.hpp>
#include <ChronoSort/Util/AssetPaths.hpp>
#include <ChronoSort/Util/TextUtil.hpp>

#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include <algorithm>
#include <fstream>

namespace ChronoSort::Scene {

    namespace {
        namespace BuiltIn = Tsukino::BuiltIn::ECS;

        constexpr int   kLinesPerPage = 15;
        constexpr int   kLayerSheet   = 30;
        constexpr float kLineHeight   = 37.0f;
        constexpr float kLineX        = -306.0f;    // 行の書き出し（紙の内側の左端の少し内）
        constexpr float kFirstLineY   = -282.0f;    // 1行目の高さ（見出しの下）

        //! Credits.json の1件分です。
        struct CreditEntry {
            std::string id;
            std::string title;
            std::string file;
            std::string page;
            std::string artist;
            std::string license;
            std::string licenseUrl;

            template <class Archive>
            void serialize(Archive& archive) {
                archive(cereal::make_nvp("id", id), cereal::make_nvp("title", title), cereal::make_nvp("file", file), cereal::make_nvp("page", page),
                        cereal::make_nvp("artist", artist), cereal::make_nvp("license", license), cereal::make_nvp("licenseUrl", licenseUrl));
            }
        };
    }    // namespace

    //----------------------------------------------------------------------------
    //! シーンを初期化します。
    //----------------------------------------------------------------------------
    void CreditsScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI&) {
        auto* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        m_stage.Build(m_scene, Game::Tuning().Stage().creditsCards);
        m_ui = std::make_unique<UI::PaperUI>(m_scene, *context->assetManager);
        LoadLines();
        m_pageCount = std::max(1, static_cast<int>((m_lines.size() + kLinesPerPage - 1) / kLinesPerPage));

        const UI::PaperLayout layout(Util::AssetPaths::UILayout("Credits"));
        m_sheet = m_ui->Paper(layout.Spec("sheet"));
        m_ui->SlideIn(m_sheet, hlslpp::float2(0.0f, 900.0f), 0.1f, 0.6f);
        m_ui->Text(L"出典・クレジット", hlslpp::float2(0.0f, -325.0f), 1.05f, UI::kInkColor, kLayerSheet + 1, m_sheet);
        m_pageNo = m_ui->Text(L"", hlslpp::float2(0.0f, 335.0f), 0.5f, UI::kFadedInkColor, kLayerSheet + 1, m_sheet);

        auto button = [&](const char* name, const std::wstring& text) {
            const auto paper = UI::NoteButton(*m_ui, layout.Spec(name), text, 0.72f);
            m_ui->SlideIn(paper, hlslpp::float2(260.0f, 0.0f), 0.5f);
            return paper;
        };
        m_next = button("next", L"次の頁");
        m_prev = button("prev", L"前の頁");
        m_back = button("back", L"机に戻る");
        ShowPage(0, 0);
    }

    //----------------------------------------------------------------------------
    //! Credits.json などから表示する行を作ります。
    //----------------------------------------------------------------------------
    void CreditsScene::LoadLines() {
        std::vector<CreditEntry> images;
        std::vector<CreditEntry> audio;
        std::ifstream            is(Util::AssetPaths::kCredits);
        if(is) {
            try {
                cereal::JSONInputArchive archive(is);
                archive(cereal::make_nvp("images", images), cereal::make_nvp("audio", audio));
            } catch(const std::exception& e) {
                Tsukino::Core::Log::Warn(std::string("Failed to read Credits.json: ") + e.what());
            }
        }

        m_lines.push_back({L"制作", true});
        m_lines.push_back({L"ゲームエンジン  TsukinoEngine"});
        m_lines.push_back({L"ルール  出来事を年代順に並べる定番の遊び方をもとに構成"});
        m_lines.push_back({L"文字  Noto Sans CJK JP / Noto Serif JP（SIL Open Font License 1.1）"});
        m_lines.push_back({L"効果音  このゲームのためにコードで合成"});
        m_lines.push_back({L"年号は一般的な教科書の表記に従っています", false, true});

        if(!audio.empty()) {
            m_lines.push_back({L"", false});
            m_lines.push_back({L"音楽", true});
            for(const CreditEntry& entry : audio) {
                m_lines.push_back({Util::Utf8ToWide(entry.title + "  ― " + entry.license)});
                m_lines.push_back({Util::Utf8ToWide("演奏 " + entry.artist + " / " + entry.file), false, true});
            }
        }

        m_lines.push_back({L"", false});
        m_lines.push_back({L"カードの絵（Wikimedia Commons）", true});
        for(const CreditEntry& entry : images) {
            m_lines.push_back({Util::Utf8ToWide(entry.title + "  ― " + entry.license)});
            m_lines.push_back({Util::Utf8ToWide(entry.artist + " / " + entry.file), false, true});
        }
    }

    //----------------------------------------------------------------------------
    //! 指定した頁を表示します。
    //----------------------------------------------------------------------------
    void CreditsScene::ShowPage(int page, int direction) {
        m_current = std::clamp(page, 0, m_pageCount - 1);

        // 新しい頁は、めくる向きの反対側から滑り込ませる
        if(m_page != entt::null) {
            m_ui->Destroy(m_page);
        }
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        m_page                           = m_scene.CreateEntity();
        auto& transform                  = registry.AddComponent<BuiltIn::TransformComponent>(m_page);
        transform.parent                 = m_sheet;
        // 頁は紙と同じ位置に重ねる入れ物なので、紙の内側の四角を写し、行を紙の内側に収める
        registry.AddComponent<ECS::PaperComponent>(m_page, registry.GetComponent<ECS::PaperComponent>(m_sheet));

        const size_t first = static_cast<size_t>(m_current) * kLinesPerPage;
        for(size_t i = first; i < std::min(m_lines.size(), first + kLinesPerPage); ++i) {
            const Line& line = m_lines[i];
            const float y    = kFirstLineY + static_cast<float>(i - first) * kLineHeight;
            const float size = line.heading ? 0.7f : (line.faded ? 0.42f : 0.52f);
            const auto  color = line.heading ? UI::kSealRedColor : (line.faded ? UI::kFadedInkColor : UI::kInkColor);
            // 長い作者名やファイル名は、PaperUI::Text が紙の内側の幅まで縮める
            m_ui->Text(line.text, hlslpp::float2(line.faded ? kLineX + 18.0f : kLineX, y), size, color, kLayerSheet + 2, m_page, false);
        }
        if(direction != 0) {
            m_ui->SlideIn(m_page, hlslpp::float2(220.0f * static_cast<float>(direction), 0.0f), 0.0f, 0.35f);
        }
        m_ui->SetText(m_pageNo, std::to_wstring(m_current + 1) + L" / " + std::to_wstring(m_pageCount));
    }

    //----------------------------------------------------------------------------
    //! シーンを更新します。
    //----------------------------------------------------------------------------
    void CreditsScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        auto*            context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        Game::SoundBank& sounds  = Game::Services().Sounds();
        auto&            input   = *context->inputSystem;
        m_stage.Update(deltaTime);

        const bool next = m_ui->Clicked(m_next) || input.IsKeyPressed(Tsukino::Input::KeyCode::Right) || input.GetWheelDelta() < 0.0f;
        const bool prev = m_ui->Clicked(m_prev) || input.IsKeyPressed(Tsukino::Input::KeyCode::Left) || input.GetWheelDelta() > 0.0f;
        if(next && m_current + 1 < m_pageCount) {
            sounds.Play("ui.close");
            ShowPage(m_current + 1, 1);
        } else if(prev && m_current > 0) {
            sounds.Play("ui.close");
            ShowPage(m_current - 1, -1);
        }
        if(m_ui->Clicked(m_back) || input.IsKeyPressed(Tsukino::Input::KeyCode::Escape)) {
            sounds.Play("ui.button");
            Router::ToTitle(api);
            return;
        }
        m_scene.Update(deltaTime);
    }

    //----------------------------------------------------------------------------
    //! シーンの終了処理を行います。
    //----------------------------------------------------------------------------
    void CreditsScene::OnExit() {
    }

}    // namespace ChronoSort::Scene
