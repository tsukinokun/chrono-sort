//----------------------------------------------------------------------------
//! @file   CardMotion.cpp
//! @brief  カードの動きを指示する関数群の実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/CardMotion.hpp>

namespace ChronoSort::ECS::CardMotion {

    //----------------------------------------------------------------------------
    //! 目標の姿勢を設定します。
    //----------------------------------------------------------------------------
    void SetTarget(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const hlslpp::float3& position, const hlslpp::quaternion& rotation, float scale) {
        auto& motion          = registry.GetComponent<CardMotionComponent>(entity);
        motion.targetPosition = position;
        motion.targetRotation = rotation;
        motion.targetScale    = scale;
    }

    //----------------------------------------------------------------------------
    //! 追従の速さを設定します。
    //----------------------------------------------------------------------------
    void SetSharpness(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, float positionSharpness, float rotationSharpness) {
        auto& motion             = registry.GetComponent<CardMotionComponent>(entity);
        motion.positionSharpness = positionSharpness;
        motion.rotationSharpness = rotationSharpness;
    }

    //----------------------------------------------------------------------------
    //! 現在地から目標へ、放物線を描いて飛ばします。
    //----------------------------------------------------------------------------
    void Fly(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, float duration, float arc, float delay) {
        auto& motion           = registry.GetComponent<CardMotionComponent>(entity);
        motion.flying          = true;
        motion.flightTime      = -delay;
        motion.flightLength    = duration;
        motion.flightArc       = arc;
        motion.flightFrom      = motion.position;
        motion.flightFromRot   = motion.rotation;
        motion.flightFromScale = motion.scale;
    }

    //----------------------------------------------------------------------------
    //! 左右に細かく震わせます。
    //----------------------------------------------------------------------------
    void Shake(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, float duration, float amplitude) {
        auto& motion          = registry.GetComponent<CardMotionComponent>(entity);
        motion.shakeTime      = duration;
        motion.shakeLength    = duration;
        motion.shakeAmplitude = amplitude;
    }

    //----------------------------------------------------------------------------
    //! 一瞬大きくして弾ませます。
    //----------------------------------------------------------------------------
    void Punch(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, float duration, float amount) {
        auto& motion       = registry.GetComponent<CardMotionComponent>(entity);
        motion.punchTime   = duration;
        motion.punchLength = duration;
        motion.punchAmount = amount;
    }

    //----------------------------------------------------------------------------
    //! 飛行中かを返します。
    //----------------------------------------------------------------------------
    bool IsFlying(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity) {
        return registry.GetComponent<CardMotionComponent>(entity).flying;
    }

}    // namespace ChronoSort::ECS::CardMotion
