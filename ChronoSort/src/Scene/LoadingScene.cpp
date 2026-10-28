//----------------------------------------------------------------------------
//! @file   LoadingScene.cpp
//! @brief  読込画面の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Scene/LoadingScene.hpp>

#include <ChronoSort/ECS/SystemSetup.hpp>
#include <ChronoSort/Game/AssetManifest.hpp>
#include <ChronoSort/Game/FrameMonitor.hpp>
#include <ChronoSort/Game/Prefabs.hpp>
#include <ChronoSort/UI/PaperLayout.hpp>
#include <ChronoSort/UI/Widgets.hpp>

#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIRectComponent.hpp>
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Material/MaterialAsset.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>
#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Renderer/RenderResources.hpp>
#include <Tsukino/Renderer/Renderer.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

namespace ChronoSort::Scene {

    namespace {
        namespace BuiltIn = Tsukino::BuiltIn::ECS;

        constexpr float kMinimumSeconds = 0.6f;     // 一瞬だけ点滅して見えないよう、最低限出しておく時間
        constexpr float kUploadBudget   = 0.004f;   // 1フレームで GPU 転送に使ってよい時間（秒）
        constexpr float kLoadShare      = 0.85f;    // 進み具合のうち、裏読みが占める割合（残りが GPU 転送）
        constexpr float kLineWidth      = 420.0f;   // 進み具合の線の最大の長さ
    }    // namespace

    //----------------------------------------------------------------------------
    //! 読み終えたあとに進むシーンを作る関数を受け取ります。
    //----------------------------------------------------------------------------
    LoadingScene::LoadingScene(std::function<std::unique_ptr<Tsukino::EngineIntegration::GameSceneBase>()> next)
        : m_next(std::move(next)) {
    }

    //----------------------------------------------------------------------------
    //! シーンを初期化します。
    //----------------------------------------------------------------------------
    void LoadingScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI&) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        auto*                   context  = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        context->renderer->SetClearColor(0.12f, 0.08f, 0.06f, 1.0f);

        ECS::AddUIOnlySystems(m_scene);
        // 画面 UI のカメラだけのステージ
        Game::Prefabs::SpawnGroup(registry, Game::Prefabs::kUIOnlyStage);

        // この画面で使う素材（紙片と墨の線）だけは同期で読む。小さいので待ちはほぼ無い
        m_ui = std::make_unique<UI::PaperUI>(m_scene, *context->assetManager);
        const UI::PaperSpec note  = UI::PaperLayout("ChronoSort/Assets/Data/UI/Loading.json").Spec("note");
        const auto          paper = m_ui->Paper(note);
        m_ui->Text(L"史料を整理しています", hlslpp::float2(0.0f, -36.0f), 0.85f, UI::kInkColor, note.sortOrder + 1, paper);
        m_percent = m_ui->Text(L"0%", hlslpp::float2(222.0f, 30.0f), 0.5f, UI::kFadedInkColor, note.sortOrder + 1, paper);

        // 墨の線は左端を基準に伸ばす（スプライトの左右の塗り）
        m_line = UI::InkLine(*m_ui, paper, "ink_line.png", hlslpp::float2(kLineWidth, 22.0f), note.sortOrder + 1, hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f), 0.0f);
        registry.GetComponent<BuiltIn::TransformComponent>(m_line).position = hlslpp::float3(-kLineWidth * 0.5f - 20.0f, 30.0f, 0.0f);
        m_ui->SlideIn(paper, hlslpp::float2(0.0f, 60.0f), 0.0f, 0.4f);

        m_queue = std::make_unique<Tsukino::Asset::AssetLoadQueue>(*context->assetManager);
        for(const std::string& path : Game::CollectStartupAssets()) {
            m_queue->Enqueue(Tsukino::Core::Path(path));
        }
        m_queue->Start();
        Game::FrameMonitor::Mark("loading");
    }

    //----------------------------------------------------------------------------
    //! シーンを更新します。
    //----------------------------------------------------------------------------
    void LoadingScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        m_elapsed += deltaTime;

        // 裏で読み終えたものを受け取り、本体スレッドで少しずつ GPU へ送る
        for(const Tsukino::Asset::AssetHandle& handle : m_queue->TakeLoaded()) {
            m_pendingUploads.push_back(handle);
        }
        UploadTextures(kUploadBudget);

        // 進み具合：裏読みが大半、残りを GPU 転送で埋める
        const size_t total    = m_uploaded + m_pendingUploads.size();
        const float  upload   = total > 0 ? static_cast<float>(m_uploaded) / static_cast<float>(total) : 1.0f;
        const float  progress = m_queue->IsFinished() ? kLoadShare + (1.0f - kLoadShare) * upload : m_queue->Progress() * kLoadShare;
        m_shown += (progress - m_shown) * std::min(1.0f, deltaTime * 10.0f);

        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        UI::SetInkLineFill(registry, m_line, m_shown);
        m_ui->SetText(m_percent, std::to_wstring(static_cast<int>(std::round(m_shown * 100.0f))) + L"%");

        const bool done = m_queue->IsFinished() && m_pendingUploads.empty();
        if(done && !m_leaving && m_elapsed >= kMinimumSeconds && m_shown > 0.98f) {
            m_leaving = true;
            Game::FrameMonitor::Mark("loading done");
            api.ChangeScene(m_next());
            return;
        }
        m_scene.Update(deltaTime);
    }

    //----------------------------------------------------------------------------
    //! 読み終えたテクスチャを、時間の予算の範囲で GPU へ転送します。
    //----------------------------------------------------------------------------
    void LoadingScene::UploadTextures(float budgetSeconds) {
        auto* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        auto& assets  = *context->assetManager;
        auto& gpu     = context->renderer->GetResources();

        auto upload = [&](const Tsukino::Asset::AssetHandle& handle) {
            if(!handle.IsValid()) {
                return;
            }
            const auto asset = assets.Get(handle);
            if(asset && asset->GetType() == Tsukino::Asset::AssetType::Texture) {
                // 初回の取得で GPU に転送され、以降は同じハンドルの描画がキャッシュを引く
                gpu.GetTextureSRV(*std::static_pointer_cast<Tsukino::Asset::TextureAsset>(asset));
            }
        };

        const auto start = std::chrono::steady_clock::now();
        while(!m_pendingUploads.empty()) {
            const Tsukino::Asset::AssetHandle handle = m_pendingUploads.front();
            m_pendingUploads.pop_front();
            ++m_uploaded;

            const auto asset = assets.Get(handle);
            if(asset && asset->GetType() == Tsukino::Asset::AssetType::Material) {
                // マテリアルは参照しているテクスチャを送る
                const auto material = std::static_pointer_cast<Tsukino::Asset::MaterialAsset>(asset);
                upload(material->albedoHandle);
                upload(material->normalHandle);
                upload(material->metallicRoughnessHandle);
                upload(material->emissiveHandle);
                upload(material->aoHandle);
            } else {
                upload(handle);
            }

            if(std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count() >= budgetSeconds) {
                break;
            }
        }
    }

    //----------------------------------------------------------------------------
    //! シーンの終了処理を行います。
    //----------------------------------------------------------------------------
    void LoadingScene::OnExit() {
        // 途中で抜けた場合も、キューのデストラクタが裏スレッドを止めて待つ
        m_queue.reset();
    }

}    // namespace ChronoSort::Scene
