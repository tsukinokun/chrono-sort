//----------------------------------------------------------------------------
//! @file   DeskStage.cpp
//! @brief  メニュー画面の背景の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Scene/DeskStage.hpp>

#include <ChronoSort/Board/BoardView.hpp>
#include <ChronoSort/ECS/CardMotion.hpp>
#include <ChronoSort/ECS/System/CardGlowSystem.hpp>
#include <ChronoSort/ECS/System/CardMotionSystem.hpp>
#include <ChronoSort/ECS/System/ParticleSystem.hpp>
#include <ChronoSort/Rules/Deck.hpp>
#include <ChronoSort/UI/PaperUI.hpp>

#include <Tsukino/BuiltIn/ECS/Component/BloomComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/PointLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SkyAtmosphereComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/EngineIntegration/ECS/System/BloomSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/InteractionSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/LightSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/ModelSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SkyAtmosphereSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/UITweenSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/WorldAnchorSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <algorithm>
#include <cmath>

namespace ChronoSort::Scene {

    namespace {
        namespace BuiltIn = Tsukino::BuiltIn::ECS;
        constexpr float kPi = 3.14159265f;
    }    // namespace

    //----------------------------------------------------------------------------
    //! 背景を作ります。
    //----------------------------------------------------------------------------
    void DeskStage::Build(Tsukino::ECS::Scene& scene, int cardCount) {
        m_scene = &scene;
        m_rng.seed(std::random_device{}());
        Tsukino::ECS::Registry& registry = scene.GetRegistry();
        auto*                   context  = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();

        scene.AddSystem(std::make_shared<BuiltIn::InteractionSystem>(), -3);
        scene.AddSystem(std::make_shared<BuiltIn::UITweenSystem>(), -2);
        scene.AddSystem(std::make_shared<ECS::CardMotionSystem>(), -1);
        scene.AddSystem(std::make_shared<ECS::ParticleSystem>(), -1);
        scene.AddSystem(std::make_shared<ECS::CardGlowSystem>(), -1);
        scene.AddSystem(std::make_shared<BuiltIn::TransformSystem>(), 0);
        scene.AddSystem(std::make_shared<BuiltIn::CameraSystem>(), 5);
        scene.AddSystem(std::make_shared<BuiltIn::WorldAnchorSystem>(), 6);
        scene.AddSystem(std::make_shared<BuiltIn::FontRendererSystem>(), 9);
        scene.AddSystem(std::make_shared<BuiltIn::SpriteRenderSystem>(), 10);
        scene.AddSystem(std::make_shared<BuiltIn::ModelSystem>(), 10);
        scene.AddSystem(std::make_shared<BuiltIn::LightSystem>(), 13);
        scene.AddSystem(std::make_shared<BuiltIn::SkyAtmosphereSystem>(), 14);
        scene.AddSystem(std::make_shared<BuiltIn::BloomSystem>(), 15);

        m_camera = scene.CreateEntity();
        registry.AddComponent<BuiltIn::TransformComponent>(m_camera);
        {
            auto& cam     = registry.AddComponent<BuiltIn::CameraComponent>(m_camera);
            cam.fov       = 38.0f;
            cam.nearZ     = 10.0f;
            cam.farZ      = 5000.0f;
            cam.useLookAt = true;
            cam.isPrimary = true;
        }
        {
            Tsukino::ECS::Entity ui = scene.CreateEntity();
            registry.AddComponent<BuiltIn::TransformComponent>(ui).position = hlslpp::float3(0.0f, 0.0f, -1.0f);
            auto& cam               = registry.AddComponent<BuiltIn::CameraComponent>(ui);
            cam.projectionType      = BuiltIn::CameraComponent::ProjectionType::Orthographic;
            cam.referenceResolution = hlslpp::float2(UI::kReferenceWidth, UI::kReferenceHeight);
            cam.isPrimary           = false;
        }
        {
            auto& sun      = registry.AddComponent<BuiltIn::DirectionalLightComponent>(scene.CreateEntity());
            sun.direction  = hlslpp::normalize(hlslpp::float3(-0.4f, -1.0f, 0.35f));
            sun.color      = hlslpp::float3(1.0f, 0.9f, 0.78f);
            sun.intensity  = 2.0f;
            sun.castShadow = true;
        }
        {
            Tsukino::ECS::Entity lamp = scene.CreateEntity();
            registry.AddComponent<BuiltIn::TransformComponent>(lamp).position = hlslpp::float3(-120.0f, 240.0f, 60.0f);
            auto& point     = registry.AddComponent<BuiltIn::PointLightComponent>(lamp);
            point.color     = hlslpp::float3(1.0f, 0.72f, 0.42f);
            point.intensity = 9.0f;
            point.range     = 800.0f;
        }
        registry.AddComponent<BuiltIn::SkyAtmosphereComponent>(scene.CreateEntity());
        registry.AddComponent<BuiltIn::BloomComponent>(scene.CreateEntity()).intensity = 0.45f;

        // 卓と、研究の途中で散らかしたようなカード
        m_factory = std::make_unique<Board::CardFactory>(scene, *context->assetManager);
        m_factory->CreateTable();
        std::vector<Rules::CardData> cards = Rules::LoadAllCards();
        std::shuffle(cards.begin(), cards.end(), m_rng);
        std::uniform_real_distribution<float> unit(0.0f, 1.0f);
        for(int i = 0; i < cardCount && i < static_cast<int>(cards.size()); ++i) {
            const float          x        = (unit(m_rng) * 2.0f - 1.0f) * 520.0f;
            const float          z        = -120.0f + unit(m_rng) * 460.0f;
            const float          y        = Board::kTableSurfaceY + static_cast<float>(i) * 0.7f;
            const bool           revealed = unit(m_rng) < 0.5f;
            hlslpp::quaternion   rotation = hlslpp::quaternion::rotation_y((unit(m_rng) * 2.0f - 1.0f) * kPi * 0.35f);
            if(revealed) {
                rotation = hlslpp::mul(rotation, Board::RevealedRotation());
            }
            // 上から舞い落ちてくるように配置する
            const Tsukino::ECS::Entity card = m_factory->CreateCard(cards[static_cast<size_t>(i)], hlslpp::float3(x * 0.6f, 420.0f + unit(m_rng) * 200.0f, z - 200.0f), rotation);
            ECS::CardMotion::SetTarget(registry, card, hlslpp::float3(x, y, z), rotation);
            ECS::CardMotion::Fly(registry, card, 0.9f + unit(m_rng) * 0.4f, 40.0f, 0.1f + static_cast<float>(i) * 0.07f);
        }
        Update(0.0f);
    }

    //----------------------------------------------------------------------------
    //! カメラを漂わせます。
    //----------------------------------------------------------------------------
    void DeskStage::Update(float deltaTime) {
        if(m_scene == nullptr) {
            return;
        }
        m_time += deltaTime;
        Tsukino::ECS::Registry& registry = m_scene->GetRegistry();
        auto&                   transform = registry.GetComponent<BuiltIn::TransformComponent>(m_camera);
        auto&                   camera    = registry.GetComponent<BuiltIn::CameraComponent>(m_camera);

        // 机の上をゆっくり見回す（ほとんど止まって見えるくらいの速さ）
        const float angle   = std::sin(m_time * 0.07f) * 0.35f;
        const float height  = 430.0f + std::sin(m_time * 0.11f) * 25.0f;
        transform.position  = hlslpp::float3(std::sin(angle) * 520.0f, height, -std::cos(angle) * 520.0f + 80.0f);
        transform.dirty     = true;
        camera.lookAtTarget = hlslpp::float3(std::sin(m_time * 0.05f) * 60.0f, 0.0f, 90.0f);
        camera.dirty        = true;
    }

}    // namespace ChronoSort::Scene
