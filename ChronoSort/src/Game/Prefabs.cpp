//----------------------------------------------------------------------------
//! @file   Prefabs.cpp
//! @brief  ゲームの Prefab の生成の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Game/Prefabs.hpp>

#include <ChronoSort/ECS/Serialization/CardMotionComponentSerialization.hpp>

#include <Tsukino/Engine/ECS/Prefab/PrefabFactory.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

namespace ChronoSort::Game::Prefabs {

    namespace {

        //----------------------------------------------------------------------------
        //! レジストリが持つエンジンのコンテキストから PrefabFactory を返します。
        //! @param  [in] registry レジストリ。
        //! @return PrefabFactory。
        //----------------------------------------------------------------------------
        Tsukino::Engine::ECS::Prefab::PrefabFactory& Factory(Tsukino::ECS::Registry& registry) {
            return *registry.GetContext<Tsukino::EngineIntegration::EngineContext*>()->prefabFactory;
        }

    }    // namespace

    //----------------------------------------------------------------------------
    //! ゲームのコンポーネントを PrefabFactory に登録します。
    //----------------------------------------------------------------------------
    void RegisterComponents(Tsukino::Engine::ECS::Prefab::PrefabFactory& factory) {
        factory.RegisterComponent<ECS::CardMotionComponent>("CardMotionComponent");
    }

    //----------------------------------------------------------------------------
    //! Prefab から1体を生成します。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity Spawn(Tsukino::ECS::Registry& registry, const char* path) {
        return Factory(registry).Instantiate(path, registry);
    }

    //----------------------------------------------------------------------------
    //! 名前付きのまとまりを生成します。
    //----------------------------------------------------------------------------
    StageInstance SpawnGroup(Tsukino::ECS::Registry& registry, const char* path) {
        StageInstance stage;
        for(const auto& [name, entity] : Factory(registry).InstantiateGroup(std::string(path), registry)) {
            stage.emplace(name, entity);
        }
        return stage;
    }

}    // namespace ChronoSort::Game::Prefabs
