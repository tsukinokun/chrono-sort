//----------------------------------------------------------------------------
//! @file   LifetimeSystem.cpp
//! @brief  時間が来た UI を子ごと片付けるシステムの実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/System/LifetimeSystem.hpp>

#include <ChronoSort/ECS/Component/LifetimeComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <vector>

namespace ChronoSort::ECS {

    //----------------------------------------------------------------------------
    //! 残り時間を減らして片付けます。
    //----------------------------------------------------------------------------
    void LifetimeSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        // 破棄は子孫を探すために全体を見るので、時間が来たものを先に集めてから行う
        std::vector<Tsukino::ECS::Entity> expired;
        registry.View<LifetimeComponent>().each([&](Tsukino::ECS::Entity entity, LifetimeComponent& lifetime) {
            lifetime.remaining -= deltaTime;
            if(lifetime.remaining <= 0.0f) {
                expired.push_back(entity);
            }
        });
        for(const Tsukino::ECS::Entity entity : expired) {
            registry.RemoveComponent<LifetimeComponent>(entity);
            DestroyTree(registry, entity);
        }
    }

    //----------------------------------------------------------------------------
    //! エンティティと、Transform で子にしているものをまとめて破棄します。
    //----------------------------------------------------------------------------
    void DestroyTree(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity) {
        if(entity == entt::null) {
            return;
        }
        std::vector<Tsukino::ECS::Entity> pending = {entity};
        // 子は親を指しているだけなので、親から辿れない。全体を見て子孫を集める
        for(size_t i = 0; i < pending.size(); ++i) {
            const Tsukino::ECS::Entity current = pending[i];
            registry.View<Tsukino::BuiltIn::ECS::TransformComponent>().each(
                [&](Tsukino::ECS::Entity child, const Tsukino::BuiltIn::ECS::TransformComponent& transform) {
                    if(static_cast<Tsukino::ECS::Entity>(transform.parent) == current) {
                        pending.push_back(child);
                    }
                });
        }
        for(const Tsukino::ECS::Entity e : pending) {
            registry.QueueDestroy(e);
        }
    }

}    // namespace ChronoSort::ECS
