//----------------------------------------------------------------------------
//! @file   SystemSetup.cpp
//! @brief  シーンに並べるシステムの組み合わせの実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/SystemSetup.hpp>

#include <ChronoSort/ECS/System/BoardLayoutSystem.hpp>
#include <ChronoSort/ECS/System/CardGlowSystem.hpp>
#include <ChronoSort/ECS/System/CardMotionSystem.hpp>
#include <ChronoSort/ECS/System/CountUpTextSystem.hpp>
#include <ChronoSort/ECS/System/CameraRigSystem.hpp>
#include <ChronoSort/ECS/System/CpuTurnSystem.hpp>
#include <ChronoSort/ECS/System/HumanInputSystem.hpp>
#include <ChronoSort/ECS/System/MatchFeedbackSystem.hpp>
#include <ChronoSort/ECS/System/MatchFlowSystem.hpp>
#include <ChronoSort/ECS/System/RevealSystem.hpp>

#include <Tsukino/Engine/ECS/Scene.hpp>
#include <Tsukino/EngineIntegration/ECS/System/BloomSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/BurstParticleSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/InteractionSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/LifetimeSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/LightSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/ModelSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SkyAtmosphereSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/UITweenSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/WorldAnchorSystem.hpp>

#include <memory>

namespace ChronoSort::ECS {

    namespace {
        namespace BuiltIn = Tsukino::BuiltIn::ECS;
    }    // namespace

    //----------------------------------------------------------------------------
    //! 3D の卓の画面のシステムを並べます。
    //----------------------------------------------------------------------------
    void AddTableSystems(Tsukino::ECS::Scene& scene) {
        scene.AddSystem(std::make_shared<BuiltIn::InteractionSystem>(), Order::kInput);
        scene.AddSystem(std::make_shared<BuiltIn::UITweenSystem>(), Order::kUIAnimation);
        scene.AddSystem(std::make_shared<BuiltIn::LifetimeSystem>(), Order::kUIAnimation);
        // 並べ方が目標を書き、CardMotionSystem がそこへ動かす（同じ順番の中では登録した順に更新される）
        scene.AddSystem(std::make_shared<BoardLayoutSystem>(), Order::kGameplay);
        scene.AddSystem(std::make_shared<CardMotionSystem>(), Order::kGameplay);
        scene.AddSystem(std::make_shared<BuiltIn::BurstParticleSystem>(), Order::kGameplay);
        scene.AddSystem(std::make_shared<CardGlowSystem>(), Order::kGameplay);
        scene.AddSystem(std::make_shared<BuiltIn::TransformSystem>(), Order::kTransform);
        scene.AddSystem(std::make_shared<BuiltIn::CameraSystem>(), Order::kCamera);
        scene.AddSystem(std::make_shared<BuiltIn::WorldAnchorSystem>(), Order::kAnchoredUI);
        scene.AddSystem(std::make_shared<BuiltIn::FontRendererSystem>(), Order::kText);
        scene.AddSystem(std::make_shared<BuiltIn::SpriteRenderSystem>(), Order::kDraw);
        scene.AddSystem(std::make_shared<BuiltIn::ModelSystem>(), Order::kDraw);
        scene.AddSystem(std::make_shared<BuiltIn::LightSystem>(), Order::kLights);
        scene.AddSystem(std::make_shared<BuiltIn::SkyAtmosphereSystem>(), Order::kSky);
        scene.AddSystem(std::make_shared<BuiltIn::BloomSystem>(), Order::kPostProcess);
    }

    //----------------------------------------------------------------------------
    //! 対局を進めるシステムを並べます。
    //----------------------------------------------------------------------------
    void AddMatchSystems(Tsukino::ECS::Scene& scene) {
        // 段取り → CPU → 人間の入力 → 公開の演出 → カメラの順（出したカードを同じフレームのうちに運び始める）
        scene.AddSystem(std::make_shared<MatchFlowSystem>(), Order::kMatch);
        scene.AddSystem(std::make_shared<CpuTurnSystem>(), Order::kMatch);
        scene.AddSystem(std::make_shared<HumanInputSystem>(), Order::kMatch);
        scene.AddSystem(std::make_shared<RevealSystem>(), Order::kMatch);
        scene.AddSystem(std::make_shared<CameraRigSystem>(), Order::kMatch);
        scene.AddSystem(std::make_shared<MatchFeedbackSystem>(scene.GetRegistry(), scene.GetEventBus()), Order::kMatch);
        // 画面 UI の得点の数え上げ
        scene.AddSystem(std::make_shared<CountUpTextSystem>(), Order::kUIAnimation);
    }

    //----------------------------------------------------------------------------
    //! 画面 UI だけの画面のシステムを並べます。
    //----------------------------------------------------------------------------
    void AddUIOnlySystems(Tsukino::ECS::Scene& scene) {
        scene.AddSystem(std::make_shared<BuiltIn::UITweenSystem>(), Order::kUIAnimation);
        scene.AddSystem(std::make_shared<BuiltIn::LifetimeSystem>(), Order::kUIAnimation);
        scene.AddSystem(std::make_shared<BuiltIn::TransformSystem>(), Order::kTransform);
        scene.AddSystem(std::make_shared<BuiltIn::CameraSystem>(), Order::kCamera);
        scene.AddSystem(std::make_shared<BuiltIn::FontRendererSystem>(), Order::kText);
        scene.AddSystem(std::make_shared<BuiltIn::SpriteRenderSystem>(), Order::kDraw);
    }

}    // namespace ChronoSort::ECS
