//----------------------------------------------------------------------------
//! @file   ParticleComponent.hpp
//! @brief  粒子1つ分の状態
//! @detail 粒子は World 空間のスプライト（カメラを向く板）です。ParticleSystem が動かし、
//!         寿命が来たら破棄します。生成は Effects::ParticleBurst の関数を使います。
//----------------------------------------------------------------------------
#pragma once
#include <hlsl++.h>

namespace ChronoSort::ECS {

    //! 粒子1つ分の状態です。
    struct ParticleComponent {
        hlslpp::float3 velocity   = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 速度（1秒あたり）
        float          gravity    = 0.0f;     // 下向きの加速度（負なら浮き上がる）
        float          drag       = 0.0f;     // 速度の減衰（1秒あたりの割合）
        float          age        = 0.0f;     // 経過時間（負の間は待ち）
        float          life       = 1.0f;     // 寿命（秒）
        float          startSize  = 8.0f;     // 生まれたときの大きさ（ワールド単位）
        float          endSize    = 0.0f;     // 消えるときの大きさ
        hlslpp::float4 startColor = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);    // 生まれたときの色（HDR 可）
        hlslpp::float4 endColor   = hlslpp::float4(1.0f, 1.0f, 1.0f, 0.0f);    // 消えるときの色
        float          texturePixels = 64.0f;    // テクスチャの一辺（ワールドの大きさへの換算に使う）
    };

}    // namespace ChronoSort::ECS
