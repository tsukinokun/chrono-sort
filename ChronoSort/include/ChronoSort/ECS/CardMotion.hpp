//----------------------------------------------------------------------------
//! @file   CardMotion.hpp
//! @brief  カードの動きを指示する関数群
//! @detail CardMotionComponent を直接いじる代わりに、ここの関数で「どこへ・どう動くか」を指示します。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/ECS/Component/CardMotionComponent.hpp>
#include <ChronoSort/Game/TuningTypes.hpp>

#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <hlsl++.h>

namespace ChronoSort::ECS::CardMotion {

    //! 目標の姿勢を設定します。カードは指数補間で滑らかに近づきます。
    //! @param  [in,out] registry レジストリ。
    //! @param  [in]     entity   カード。
    //! @param  [in]     position 目標位置。
    //! @param  [in]     rotation 目標の向き。
    //! @param  [in]     scale    目標の拡大率。
    void SetTarget(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const hlslpp::float3& position, const hlslpp::quaternion& rotation, float scale = 1.0f);

    //! 追従の速さを設定します。
    //! @param  [in,out] registry          レジストリ。
    //! @param  [in]     entity            カード。
    //! @param  [in]     positionSharpness 位置の追従の速さ。
    //! @param  [in]     rotationSharpness 回転の追従の速さ。
    void SetSharpness(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, float positionSharpness, float rotationSharpness);

    //! 現在地から目標へ、放物線を描いて飛ばします。目標は呼ぶ前に SetTarget で決めておきます。
    //! @param  [in,out] registry レジストリ。
    //! @param  [in]     entity   カード。
    //! @param  [in]     duration 所要時間（秒）。
    //! @param  [in]     arc      弧の高さ。
    //! @param  [in]     delay    飛び始めるまでの待ち（秒）。
    void Fly(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, float duration, float arc, float delay = 0.0f);

    //! 左右に細かく震わせます。
    //! @param  [in,out] registry  レジストリ。
    //! @param  [in]     entity    カード。
    //! @param  [in]     duration  長さ（秒）。
    //! @param  [in]     amplitude 振れ幅。
    void Shake(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, float duration, float amplitude);

    //! 一瞬大きくして弾ませます。
    //! @param  [in,out] registry レジストリ。
    //! @param  [in]     entity   カード。
    //! @param  [in]     duration 長さ（秒）。
    //! @param  [in]     amount   拡大率に足す最大の量。
    void Punch(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, float duration, float amount);

    //! 追従の速さを、調整値の組で設定します。
    //! @param  [in,out] registry  レジストリ。
    //! @param  [in]     entity    カード。
    //! @param  [in]     sharpness 追従の速さ。
    inline void SetSharpness(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const Game::Sharpness& sharpness) {
        SetSharpness(registry, entity, sharpness.position, sharpness.rotation);
    }

    //! 調整値の組で、放物線を描いて飛ばします。
    //! @param  [in,out] registry レジストリ。
    //! @param  [in]     entity   カード。
    //! @param  [in]     flight   飛び方。
    inline void Fly(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const Game::Flight& flight) {
        Fly(registry, entity, flight.seconds, flight.arc, flight.delay);
    }

    //! 調整値の組で、左右に細かく震わせます。
    //! @param  [in,out] registry レジストリ。
    //! @param  [in]     entity   カード。
    //! @param  [in]     tremor   震え方。
    inline void Shake(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const Game::Tremor& tremor) {
        Shake(registry, entity, tremor.seconds, tremor.amplitude);
    }

    //! 調整値の組で、一瞬大きくして弾ませます。
    //! @param  [in,out] registry レジストリ。
    //! @param  [in]     entity   カード。
    //! @param  [in]     bump     弾み方。
    inline void Punch(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const Game::Bump& bump) {
        Punch(registry, entity, bump.seconds, bump.amount);
    }

    //! 飛行中かを返します。
    //! @param  [in,out] registry レジストリ。
    //! @param  [in]     entity   カード。
    //! @return 飛行中なら true。
    bool IsFlying(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity);

}    // namespace ChronoSort::ECS::CardMotion
