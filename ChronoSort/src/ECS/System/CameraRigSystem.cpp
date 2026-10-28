//----------------------------------------------------------------------------
//! @file   CameraRigSystem.cpp
//! @brief  卓のカメラに揺れと漂いを足すシステムの実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/System/CameraRigSystem.hpp>

#include <ChronoSort/ECS/Component/CameraRigComponent.hpp>
#include <ChronoSort/Game/GameServices.hpp>
#include <ChronoSort/Game/TuningSet.hpp>

#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <algorithm>
#include <cmath>

namespace ChronoSort::ECS {

    //----------------------------------------------------------------------------
    //! 卓のカメラに、揺れと漂いを足します。
    //----------------------------------------------------------------------------
    void CameraRigSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        const Scene::StageTuning& t = Game::Tuning().Stage();
        registry.View<Tsukino::BuiltIn::ECS::TransformComponent, Tsukino::BuiltIn::ECS::CameraComponent, CameraRigComponent>().each(
            [deltaTime, &t](auto, Tsukino::BuiltIn::ECS::TransformComponent& transform, Tsukino::BuiltIn::ECS::CameraComponent& camera, CameraRigComponent& rig) {
                rig.time += deltaTime;
                // 揺れは減衰するランダムなずれ
                rig.shake = std::max(0.0f, rig.shake - deltaTime * t.shakeDecay);
                const hlslpp::float3 jolt(std::sin(rig.time * t.shakeFreqX) * rig.shake, std::cos(rig.time * t.shakeFreqY) * rig.shake * t.shakeYScale, 0.0f);
                // ゆっくり呼吸するように少しだけ漂わせ、止まった絵に見せない
                const hlslpp::float3 drift(std::sin(rig.time * t.driftFreqX) * t.driftX, std::sin(rig.time * t.driftFreqY) * t.driftY, 0.0f);

                transform.position  = rig.basePosition + jolt + drift;
                transform.dirty     = true;
                camera.lookAtTarget = rig.baseLookAt + jolt * t.shakeLookAt;
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
