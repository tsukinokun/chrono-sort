//----------------------------------------------------------------------------
//! @file   CameraRigSystem.cpp
//! @brief  卓のカメラに揺れと漂いを足すシステムの実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/System/CameraRigSystem.hpp>

#include <ChronoSort/ECS/Component/CameraRigComponent.hpp>
#include <ChronoSort/Game/GameServices.hpp>

#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <algorithm>
#include <cmath>

namespace ChronoSort::ECS {

    //----------------------------------------------------------------------------
    //! 卓のカメラに、揺れと漂いを足します。
    //----------------------------------------------------------------------------
    void CameraRigSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        registry.View<Tsukino::BuiltIn::ECS::TransformComponent, Tsukino::BuiltIn::ECS::CameraComponent, CameraRigComponent>().each(
            [deltaTime](auto, Tsukino::BuiltIn::ECS::TransformComponent& transform, Tsukino::BuiltIn::ECS::CameraComponent& camera, CameraRigComponent& rig) {
                rig.time += deltaTime;
                // 揺れは減衰するランダムなずれ
                rig.shake = std::max(0.0f, rig.shake - deltaTime * 18.0f);
                const hlslpp::float3 jolt(std::sin(rig.time * 71.0f) * rig.shake, std::cos(rig.time * 53.0f) * rig.shake * 0.6f, 0.0f);
                // ゆっくり呼吸するように少しだけ漂わせ、止まった絵に見せない
                const hlslpp::float3 drift(std::sin(rig.time * 0.23f) * 6.0f, std::sin(rig.time * 0.31f) * 4.0f, 0.0f);

                transform.position  = rig.basePosition + jolt + drift;
                transform.dirty     = true;
                camera.lookAtTarget = rig.baseLookAt + jolt * 0.5f;
                camera.dirty        = true;
            });
    }

    //----------------------------------------------------------------------------
    //! カメラを揺らします。
    //----------------------------------------------------------------------------
    void ShakeCamera(Tsukino::ECS::Registry& registry, float strength) {
        // 設定で揺れを切っていれば揺らさない（酔いやすい人向け）
        if(!Game::Services().Settings().cameraShake) {
            return;
        }
        registry.View<CameraRigComponent>().each([strength](auto, CameraRigComponent& rig) { rig.shake = std::max(rig.shake, strength); });
    }

}    // namespace ChronoSort::ECS
