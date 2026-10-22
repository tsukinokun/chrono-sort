//----------------------------------------------------------------------------
//! @file   CardMotionSystem.cpp
//! @brief  カードを目標姿勢へ滑らかに近づけるシステムの実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/System/CardMotionSystem.hpp>

#include <ChronoSort/ECS/Component/CardMotionComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <hlsl++.h>

#include <cmath>

namespace ChronoSort::ECS {

    //----------------------------------------------------------------------------
    //! カードの Transform を目標へ近づけます。
    //----------------------------------------------------------------------------
    void CardMotionSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        registry.View<Tsukino::BuiltIn::ECS::TransformComponent, CardMotionComponent>().each(
            [deltaTime](auto, Tsukino::BuiltIn::ECS::TransformComponent& transform, CardMotionComponent& motion) {
                const float positionRate = 1.0f - std::exp(-motion.positionSharpness * deltaTime);
                const float rotationRate = 1.0f - std::exp(-motion.rotationSharpness * deltaTime);

                transform.position = hlslpp::lerp(transform.position, motion.targetPosition, hlslpp::float3(positionRate));

                // q と -q は同じ回転なので、内積が負なら反転して近い側から回す
                hlslpp::quaternion target = motion.targetRotation;
                const float        dot    = transform.rotation.x * target.x + transform.rotation.y * target.y +
                                  transform.rotation.z * target.z + transform.rotation.w * target.w;
                if(dot < 0.0f) {
                    target = hlslpp::quaternion(-target.x, -target.y, -target.z, -target.w);
                }
                transform.rotation = hlslpp::normalize(hlslpp::slerp(transform.rotation, target, rotationRate));
                transform.dirty    = true;
            });
    }

}    // namespace ChronoSort::ECS
