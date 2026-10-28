//----------------------------------------------------------------------------
//! @file   DeskStage.cpp
//! @brief  メニュー画面の背景の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Scene/DeskStage.hpp>

#include <ChronoSort/Board/BoardView.hpp>
#include <ChronoSort/ECS/CardMotion.hpp>
#include <ChronoSort/ECS/SystemSetup.hpp>
#include <ChronoSort/Game/TuningSet.hpp>
#include <ChronoSort/Game/Prefabs.hpp>
#include <ChronoSort/Rules/Deck.hpp>
#include <ChronoSort/UI/PaperUI.hpp>

#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>
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

        ECS::AddTableSystems(scene);

        // カメラ・画面 UI のカメラ・光・空・Bloom は Prefab にある（カメラの動きだけは Update で書く）
        m_camera = Game::Prefabs::SpawnGroup(registry, Game::Prefabs::kDeskStage).at("Camera");

        // 卓と、研究の途中で散らかしたようなカード
        m_factory = std::make_unique<Board::CardFactory>(scene, *context->assetManager);
        m_factory->CreateTable();
        std::vector<Rules::CardData> cards = Rules::LoadAllCards();
        std::shuffle(cards.begin(), cards.end(), m_rng);
        const StageTuning&                    t = Game::Tuning().Stage();
        std::uniform_real_distribution<float> unit(0.0f, 1.0f);
        for(int i = 0; i < cardCount && i < static_cast<int>(cards.size()); ++i) {
            const float          x        = (unit(m_rng) * 2.0f - 1.0f) * t.scatterHalfX;
            const float          z        = t.scatterNearZ + unit(m_rng) * t.scatterDepth;
            const float          y        = Board::kTableSurfaceY + static_cast<float>(i) * t.scatterStack;
            const bool           revealed = unit(m_rng) < t.scatterFaceUp;
            hlslpp::quaternion   rotation = hlslpp::quaternion::rotation_y((unit(m_rng) * 2.0f - 1.0f) * kPi * t.scatterYaw);
            if(revealed) {
                rotation = hlslpp::mul(rotation, Board::RevealedRotation());
            }
            // 上から舞い落ちてくるように配置する
            const hlslpp::float3       from(x * t.dropSpreadX, t.dropHeight + unit(m_rng) * t.dropHeightRange, z - t.dropBack);
            const Tsukino::ECS::Entity card = m_factory->CreateCard(cards[static_cast<size_t>(i)], from, rotation);
            ECS::CardMotion::SetTarget(registry, card, hlslpp::float3(x, y, z), rotation);
            ECS::CardMotion::Fly(registry, card, t.dropSeconds + unit(m_rng) * t.dropSecondsRange, t.dropArc, t.dropDelay + static_cast<float>(i) * t.dropStagger);
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
        const StageTuning& t = Game::Tuning().Stage();
        const float        angle  = std::sin(m_time * t.orbitSpeed) * t.orbitAngle;
        const float        height = t.orbitHeight + std::sin(m_time * t.orbitBobSpeed) * t.orbitBob;
        transform.position  = hlslpp::float3(std::sin(angle) * t.orbitRadius, height, -std::cos(angle) * t.orbitRadius + t.orbitCenterZ);
        transform.dirty     = true;
        camera.lookAtTarget = hlslpp::float3(std::sin(m_time * t.lookAtSwaySpeed) * t.lookAtSway, 0.0f, t.lookAtZ);
        camera.dirty        = true;
    }

}    // namespace ChronoSort::Scene
