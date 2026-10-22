//----------------------------------------------------------------------------
//! @file   ParticleSystem.cpp
//! @brief  粒子を動かし、寿命が来たら破棄するシステムの実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/System/ParticleSystem.hpp>

#include <ChronoSort/ECS/Component/ParticleComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <algorithm>

namespace ChronoSort::ECS {

    //----------------------------------------------------------------------------
    //! 粒子を更新します。
    //----------------------------------------------------------------------------
    void ParticleSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        registry.View<Tsukino::BuiltIn::ECS::TransformComponent, Tsukino::BuiltIn::ECS::SpriteComponent, ParticleComponent>().each(
            [&](Tsukino::ECS::Entity entity, Tsukino::BuiltIn::ECS::TransformComponent& transform, Tsukino::BuiltIn::ECS::SpriteComponent& sprite, ParticleComponent& particle) {
                particle.age += deltaTime;
                if(particle.age < 0.0f) {
                    // 出番待ちの間は見せない
                    sprite.tintColor = hlslpp::float4(0.0f, 0.0f, 0.0f, 0.0f);
                    return;
                }
                if(particle.age >= particle.life) {
                    // 反復中なので即時ではなく破棄の予約にする
                    registry.QueueDestroy(entity);
                    return;
                }

                particle.velocity.y -= particle.gravity * deltaTime;
                particle.velocity *= std::max(0.0f, 1.0f - particle.drag * deltaTime);
                transform.position += particle.velocity * deltaTime;

                const float t    = particle.age / particle.life;
                const float size = particle.startSize + (particle.endSize - particle.startSize) * t;
                const float s    = size / particle.texturePixels;
                transform.scale  = hlslpp::float3(s, s, 1.0f);
                transform.dirty  = true;
                sprite.tintColor = hlslpp::lerp(particle.startColor, particle.endColor, hlslpp::float4(t));
            });
    }

}    // namespace ChronoSort::ECS
