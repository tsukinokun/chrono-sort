//----------------------------------------------------------------------------
//! @file   SoloGameScene.cpp
//! @brief  ソロモードの対局画面の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Scene/SoloGameScene.hpp>

#include <ChronoSort/ECS/Component/CardMotionComponent.hpp>
#include <ChronoSort/ECS/System/CardMotionSystem.hpp>
#include <ChronoSort/Scene/TitleScene.hpp>

#include <Tsukino/BuiltIn/ECS/Component/BloomComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/PointLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SkyAtmosphereComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/BloomSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/LightSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/ModelSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SkyAtmosphereSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp>
#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <entt/entt.hpp>
#include <hlsl++.h>

#include <cmath>

namespace ChronoSort::Scene {

    namespace {
        constexpr int   kHandSize        = 5;         // 手札の枚数（ターン進行を載せるまでの仮の値）
        constexpr float kTableSurfaceY   = 0.31f;     // 卓に寝かせたカードの高さ（板の厚みの半分 + 僅か）
        constexpr float kHandCenterZ     = -230.0f;   // 手札の中心（カメラ側）
        constexpr float kHandBaseY       = 90.0f;     // 手札の高さ（卓から浮かせる）
        constexpr float kHandSpacing     = 56.0f;     // 手札の間隔
        constexpr float kHandTilt        = -0.55f;    // 手札をカメラへ向ける傾き（X 軸まわり、ラジアン）
        constexpr float kHandFanAngle    = 0.07f;     // 扇の1枚あたりの開き（ラジアン）
        constexpr float kHandArcDepth    = 6.0f;      // 扇の端ほど奥へ下げる量
    }    // namespace

    //----------------------------------------------------------------------------
    //! シーンを初期化します。
    //----------------------------------------------------------------------------
    void SoloGameScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI&) {
        auto* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();

        RegisterSystems();
        CreateStage();

        m_cardFactory = std::make_unique<Board::CardFactory>(m_scene, *context->assetManager);
        m_cardFactory->CreateTable();

        m_deck.LoadFromFile("ChronoSort/Assets/Data/Decks/japan.json");
        m_deck.LoadFromFile("ChronoSort/Assets/Data/Decks/world.json");
        m_rng.seed(std::random_device{}());
        m_deck.Shuffle(m_rng);

        // 場の最初の1枚は年号を見せて置く
        if(const auto first = m_deck.Draw()) {
            m_timeline.Reset(*first);
            m_firstCard = m_cardFactory->CreateCard(*first, hlslpp::float3(0.0f, kTableSurfaceY, 40.0f), Board::RevealedRotation());
        }

        // 手札は山札の位置から配られてくるように、右奥に生成してから扇の位置へ動かす
        for(int i = 0; i < kHandSize; ++i) {
            if(const auto card = m_deck.Draw()) {
                m_handCards.push_back(m_cardFactory->CreateCard(*card, hlslpp::float3(560.0f, kTableSurfaceY + i * 0.7f, 200.0f), hlslpp::quaternion::identity()));
            }
        }
        LayoutHand();

        // 操作説明（画面 UI）
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        Tsukino::ECS::Entity    help     = m_scene.CreateEntity();
        registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(help).position = hlslpp::float3(24.0f, 20.0f, 0.0f);
        auto& font = registry.AddComponent<Tsukino::BuiltIn::ECS::FontComponent>(help);
        font.text  = L"Space: 場のカードを裏返す　Esc: タイトルへ";
        font.scale = 0.6f;
    }

    //----------------------------------------------------------------------------
    //! シーンを更新します。
    //----------------------------------------------------------------------------
    void SoloGameScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        auto*                   context  = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();

        if(context->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Escape)) {
            api.ChangeScene(std::make_unique<TitleScene>());
            return;
        }

        // 表裏の確認用：最初のカードを持ち上げずにその場で裏返す
        if(context->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Space) && m_firstCard != entt::null) {
            m_firstCardRevealed = !m_firstCardRevealed;
            registry.GetComponent<ECS::CardMotionComponent>(m_firstCard).targetRotation =
                m_firstCardRevealed ? Board::RevealedRotation() : hlslpp::quaternion::identity();
        }

        m_scene.Update(deltaTime);
    }

    //----------------------------------------------------------------------------
    //! シーンの終了処理を行います。
    //----------------------------------------------------------------------------
    void SoloGameScene::OnExit() {
    }

    //----------------------------------------------------------------------------
    //! 描画と演出に使うシステムを登録します。
    //----------------------------------------------------------------------------
    void SoloGameScene::RegisterSystems() {
        // カードの動きは Transform の計算より前に確定させる
        m_scene.AddSystem(std::make_shared<ECS::CardMotionSystem>(), -1);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), 0);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::CameraSystem>(), 5);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::FontRendererSystem>(), 9);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SpriteRenderSystem>(), 10);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::ModelSystem>(), 10);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::LightSystem>(), 13);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SkyAtmosphereSystem>(), 14);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::BloomSystem>(), 15);
    }

    //----------------------------------------------------------------------------
    //! カメラ・ライト・後処理を配置します。
    //----------------------------------------------------------------------------
    void SoloGameScene::CreateStage() {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();

        // 卓を手前上から見下ろす透視カメラ
        {
            Tsukino::ECS::Entity camera = m_scene.CreateEntity();
            registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(camera).position = hlslpp::float3(0.0f, 500.0f, -520.0f);
            auto& cam        = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(camera);
            cam.fov          = 40.0f;
            cam.nearZ        = 10.0f;
            cam.farZ         = 5000.0f;
            cam.useLookAt    = true;
            cam.lookAtTarget = hlslpp::float3(0.0f, 0.0f, -60.0f);
            cam.isPrimary    = true;
        }

        // 斜め上からの主光源。卓にカードの影を落とす
        {
            Tsukino::ECS::Entity light = m_scene.CreateEntity();
            auto&                sun   = registry.AddComponent<Tsukino::BuiltIn::ECS::DirectionalLightComponent>(light);
            sun.direction              = hlslpp::normalize(hlslpp::float3(0.35f, -1.0f, 0.45f));
            sun.color                  = hlslpp::float3(1.0f, 0.93f, 0.82f);
            sun.intensity              = 1.6f;
            sun.castShadow             = true;
        }

        // 卓の中央を温かく照らす補助光（ランプの明かり）
        {
            Tsukino::ECS::Entity lamp = m_scene.CreateEntity();
            registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(lamp).position = hlslpp::float3(0.0f, 220.0f, 0.0f);
            auto& point     = registry.AddComponent<Tsukino::BuiltIn::ECS::PointLightComponent>(lamp);
            point.color     = hlslpp::float3(1.0f, 0.75f, 0.45f);
            point.intensity = 6.0f;
            point.range     = 700.0f;
        }

        // 環境光（IBL）の元。空そのものは卓に隠れて見えない
        registry.AddComponent<Tsukino::BuiltIn::ECS::SkyAtmosphereComponent>(m_scene.CreateEntity());

        // 発光演出（正解時の金色など）を滲ませる
        {
            auto& bloom     = registry.AddComponent<Tsukino::BuiltIn::ECS::BloomComponent>(m_scene.CreateEntity());
            bloom.threshold = 1.0f;
            bloom.intensity = 0.4f;
        }
    }

    //----------------------------------------------------------------------------
    //! 手札の各カードの目標姿勢を、扇状の並びに合わせて設定します。
    //----------------------------------------------------------------------------
    void SoloGameScene::LayoutHand() {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        const float             center   = (static_cast<float>(m_handCards.size()) - 1.0f) * 0.5f;
        const hlslpp::quaternion tilt    = hlslpp::quaternion::rotation_x(kHandTilt);

        for(size_t i = 0; i < m_handCards.size(); ++i) {
            const float offset = static_cast<float>(i) - center;

            auto& motion = registry.GetComponent<ECS::CardMotionComponent>(m_handCards[i]);
            // 右のカードほど少し手前・上に置き、重なりの前後をはっきりさせる
            motion.targetPosition = hlslpp::float3(offset * kHandSpacing,
                                                   kHandBaseY + static_cast<float>(i) * 1.5f,
                                                   kHandCenterZ - offset * offset * kHandArcDepth - static_cast<float>(i) * 1.0f);
            // 扇の開き（カードの法線まわり）を先に、カメラへ向ける傾きを後に効かせる
            const hlslpp::quaternion fan = hlslpp::quaternion::rotation_y(offset * kHandFanAngle);
            motion.targetRotation        = hlslpp::mul(tilt, fan);
        }
    }

}    // namespace ChronoSort::Scene
