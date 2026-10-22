//----------------------------------------------------------------------------
//! @file   CardGlowComponent.hpp
//! @brief  カードの発光（正解の金色・不正解の赤など）
//! @detail CardGlowSystem が MaterialPropertyBlockComponent の emissive に書き込み、
//!         Bloom で滲ませます。intensity は毎秒 decay ずつ減り、0 で消えます。
//----------------------------------------------------------------------------
#pragma once
#include <hlsl++.h>

namespace ChronoSort::ECS {

    //! カードの発光です。
    struct CardGlowComponent {
        hlslpp::float3 color      = hlslpp::float3(1.0f, 0.8f, 0.3f);    // 発光色（HDR。1 を超えると Bloom で滲む）
        float          intensity  = 0.0f;    // 今の強さ
        float          decay      = 1.5f;    // 1秒あたりに減る強さ
        float          pulseSpeed = 0.0f;    // 0 より大きければ脈打つ（選択中の表示など）。減衰はしない
        float          time       = 0.0f;    // 脈打ちの経過時間
    };

}    // namespace ChronoSort::ECS
