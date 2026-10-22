//----------------------------------------------------------------------------
//! @file   CardGlowSystem.cpp
//! @brief  カードの発光を更新するシステムの実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/System/CardGlowSystem.hpp>

#include <ChronoSort/ECS/Component/CardGlowComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/MaterialPropertyBlockComponent.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace ChronoSort::ECS {

    namespace {

        //----------------------------------------------------------------------------
        //! 発光を付けるか、付いていれば置き換えます。
        //! @param  [in,out] registry レジストリ。
        //! @param  [in]     entity   カード。
        //! @param  [in]     glow     発光の設定。
        //----------------------------------------------------------------------------
        void SetGlow(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const CardGlowComponent& glow) {
            if(registry.HasComponent<CardGlowComponent>(entity)) {
                registry.GetComponent<CardGlowComponent>(entity) = glow;
            } else {
                registry.AddComponent<CardGlowComponent>(entity, glow);
            }
        }

    }    // namespace

    //----------------------------------------------------------------------------
    //! 発光を更新します。
    //----------------------------------------------------------------------------
    void CardGlowSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        std::vector<Tsukino::ECS::Entity> finished;
        registry.View<CardGlowComponent>().each([&](Tsukino::ECS::Entity entity, CardGlowComponent& glow) {
            float strength = 0.0f;
            if(glow.pulseSpeed > 0.0f) {
                glow.time += deltaTime;
                strength = glow.intensity * (0.55f + 0.45f * std::sin(glow.time * glow.pulseSpeed));
            } else {
                glow.intensity = std::max(0.0f, glow.intensity - glow.decay * deltaTime);
                strength       = glow.intensity;
                if(glow.intensity <= 0.0f) {
                    finished.push_back(entity);
                }
            }
            auto& block    = registry.HasComponent<Tsukino::BuiltIn::ECS::MaterialPropertyBlockComponent>(entity)
                                 ? registry.GetComponent<Tsukino::BuiltIn::ECS::MaterialPropertyBlockComponent>(entity)
                                 : registry.AddComponent<Tsukino::BuiltIn::ECS::MaterialPropertyBlockComponent>(entity);
            block.emissive = glow.color * strength;
        });

        // 光り終わったら上書きを外して、マテリアル本来の値に戻す
        for(Tsukino::ECS::Entity entity : finished) {
            registry.RemoveComponent<CardGlowComponent>(entity);
            registry.RemoveComponent<Tsukino::BuiltIn::ECS::MaterialPropertyBlockComponent>(entity);
        }
    }

    //----------------------------------------------------------------------------
    //! カードを光らせます。
    //----------------------------------------------------------------------------
    void Glow(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const hlslpp::float3& color, float intensity, float decay) {
        CardGlowComponent glow;
        glow.color     = color;
        glow.intensity = intensity;
        glow.decay     = decay;
        SetGlow(registry, entity, glow);
    }

    //----------------------------------------------------------------------------
    //! カードを脈打つように光らせ続けます。
    //----------------------------------------------------------------------------
    void PulseGlow(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const hlslpp::float3& color, float intensity, float speed) {
        CardGlowComponent glow;
        glow.color      = color;
        glow.intensity  = intensity;
        glow.pulseSpeed = speed;
        SetGlow(registry, entity, glow);
    }

    //----------------------------------------------------------------------------
    //! カードの発光を止めます。
    //----------------------------------------------------------------------------
    void StopGlow(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity) {
        if(registry.HasComponent<CardGlowComponent>(entity)) {
            registry.RemoveComponent<CardGlowComponent>(entity);
        }
        if(registry.HasComponent<Tsukino::BuiltIn::ECS::MaterialPropertyBlockComponent>(entity)) {
            registry.RemoveComponent<Tsukino::BuiltIn::ECS::MaterialPropertyBlockComponent>(entity);
        }
    }

}    // namespace ChronoSort::ECS
