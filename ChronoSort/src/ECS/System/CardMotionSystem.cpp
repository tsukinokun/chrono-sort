//----------------------------------------------------------------------------
//! @file   CardMotionSystem.cpp
//! @brief  カードを目標姿勢へ滑らかに近づけるシステムの実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/System/CardMotionSystem.hpp>

#include <ChronoSort/ECS/Component/CardMotionComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/Math/Easing.hpp>

#include <hlsl++.h>

#include <algorithm>
#include <cmath>

namespace ChronoSort::ECS {

    namespace {

        //----------------------------------------------------------------------------
        //! 近い側から回るように符号を揃えて slerp します。
        //! @param  [in] from 始めの向き。
        //! @param  [in] to   終わりの向き。
        //! @param  [in] t    進み具合（0〜1）。
        //! @return 補間した向き。
        //----------------------------------------------------------------------------
        hlslpp::quaternion SlerpShortest(const hlslpp::quaternion& from, hlslpp::quaternion to, float t) {
            // q と -q は同じ回転なので、内積が負なら反転して近い側から回す
            const float dot = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
            if(dot < 0.0f) {
                to = hlslpp::quaternion(-to.x, -to.y, -to.z, -to.w);
            }
            return hlslpp::normalize(hlslpp::slerp(from, to, t));
        }

    }    // namespace

    //----------------------------------------------------------------------------
    //! カードの Transform を目標へ近づけます。
    //----------------------------------------------------------------------------
    void CardMotionSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        namespace Easing = Tsukino::Core::Math::Easing;

        registry.View<Tsukino::BuiltIn::ECS::TransformComponent, CardMotionComponent>().each(
            [deltaTime](auto, Tsukino::BuiltIn::ECS::TransformComponent& transform, CardMotionComponent& motion) {
                if(!motion.initialized) {
                    motion.position    = transform.position;
                    motion.rotation    = transform.rotation;
                    motion.scale       = transform.scale.x;
                    motion.initialized = true;
                }

                if(motion.flying) {
                    motion.flightTime += deltaTime;
                    const float raw = std::clamp(motion.flightTime / std::max(motion.flightLength, 0.001f), 0.0f, 1.0f);
                    if(motion.flightTime >= 0.0f) {
                        // 出だしは素早く、着地の直前で減速する
                        const float t   = Easing::Evaluate(Easing::EaseType::InOutCubic, raw);
                        motion.position = hlslpp::lerp(motion.flightFrom, motion.targetPosition, hlslpp::float3(t));
                        motion.position.y += motion.flightArc * 4.0f * raw * (1.0f - raw);
                        motion.rotation = SlerpShortest(motion.flightFromRot, motion.targetRotation, t);
                        motion.scale    = motion.flightFromScale + (motion.targetScale - motion.flightFromScale) * t;
                    }
                    if(raw >= 1.0f) {
                        motion.flying = false;
                    }
                } else {
                    // フレームレートに依存しないよう、補間率は 1 - exp(-速さ × 経過時間)
                    const float positionRate = 1.0f - std::exp(-motion.positionSharpness * deltaTime);
                    const float rotationRate = 1.0f - std::exp(-motion.rotationSharpness * deltaTime);
                    motion.position          = hlslpp::lerp(motion.position, motion.targetPosition, hlslpp::float3(positionRate));
                    motion.rotation          = SlerpShortest(motion.rotation, motion.targetRotation, rotationRate);
                    motion.scale += (motion.targetScale - motion.scale) * positionRate;
                }

                // 演出を重ねて Transform へ書き出す
                hlslpp::float3 position = motion.position;
                float          scale    = motion.scale;
                if(motion.shakeTime > 0.0f) {
                    motion.shakeTime     = std::max(0.0f, motion.shakeTime - deltaTime);
                    const float fade     = motion.shakeTime / std::max(motion.shakeLength, 0.001f);
                    const float phase    = (motion.shakeLength - motion.shakeTime) * 55.0f;
                    position.x += std::sin(phase) * motion.shakeAmplitude * fade;
                }
                if(motion.punchTime > 0.0f) {
                    motion.punchTime = std::max(0.0f, motion.punchTime - deltaTime);
                    const float t    = 1.0f - motion.punchTime / std::max(motion.punchLength, 0.001f);
                    scale += motion.punchAmount * Easing::Evaluate(Easing::EaseType::Punch, t);
                }

                transform.position = position;
                transform.rotation = motion.rotation;
                transform.scale    = hlslpp::float3(scale, scale, scale);
                transform.dirty    = true;
            });
    }

}    // namespace ChronoSort::ECS
