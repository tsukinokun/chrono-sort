//----------------------------------------------------------------------------
//! @file   CameraRigComponent.hpp
//! @brief  卓のカメラの揺れと漂い
//! @detail カメラに付けます。Prefab で決めた姿勢を基準に、判定などでの揺れ（減衰する）と、
//!         止まった絵に見せないためのゆっくりした漂いを CameraRigSystem が足します。
//----------------------------------------------------------------------------
#pragma once
#include <hlsl++.h>

namespace ChronoSort::ECS {

    //! 卓のカメラの揺れと漂いです。
    struct CameraRigComponent {
        hlslpp::float3 basePosition = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 揺らす前の位置（Prefab の値）
        hlslpp::float3 baseLookAt   = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 揺らす前の注視点（Prefab の値）
        float          shake        = 0.0f;    // 今の揺れの強さ（毎秒減る）
        float          time         = 0.0f;    // 経過時間（揺れと漂いの位相）
    };

}    // namespace ChronoSort::ECS
