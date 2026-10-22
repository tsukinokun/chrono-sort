//----------------------------------------------------------------------------
//! @file   CardMotionComponent.hpp
//! @brief  カードの目標姿勢
//! @detail 手札・ドラッグ・場の整列など、カードの動きはすべて「目標の姿勢」を
//!         書き換えるだけにして、実際の移動は CardMotionSystem が滑らかに行います。
//----------------------------------------------------------------------------
#pragma once
#include <hlsl++.h>

namespace ChronoSort::ECS {

    //! カードの目標姿勢と追従の速さです。
    struct CardMotionComponent {
        hlslpp::float3     targetPosition    = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 目標位置
        hlslpp::quaternion targetRotation    = hlslpp::quaternion::identity();      // 目標の向き
        float              positionSharpness = 14.0f;    // 位置の追従の速さ（大きいほど速い）
        float              rotationSharpness = 10.0f;    // 回転の追従の速さ（大きいほど速い）
    };

}    // namespace ChronoSort::ECS
