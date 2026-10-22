//----------------------------------------------------------------------------
//! @file   CardMotionComponent.hpp
//! @brief  カードの目標姿勢と動きの演出
//! @detail 手札・ドラッグ・場の整列など、カードの動きはすべて「目標の姿勢」を
//!         書き換えるだけにして、実際の移動は CardMotionSystem が滑らかに行います。
//!         放物線で飛ばす・震わせる・弾ませるといった演出も、ここに積んでおけば
//!         CardMotionSystem が目標への追従と重ねて再生します。
//----------------------------------------------------------------------------
#pragma once
#include <hlsl++.h>

namespace ChronoSort::ECS {

    //! カードの目標姿勢と、再生中の演出です。
    struct CardMotionComponent {
        // 目標
        hlslpp::float3     targetPosition    = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 目標位置
        hlslpp::quaternion targetRotation    = hlslpp::quaternion::identity();      // 目標の向き
        float              targetScale       = 1.0f;     // 目標の拡大率
        float              positionSharpness = 14.0f;    // 位置の追従の速さ（大きいほど速い）
        float              rotationSharpness = 10.0f;    // 回転の追従の速さ（大きいほど速い）

        // 現在の姿勢（演出を重ねる前の値。CardMotionSystem が管理する）
        hlslpp::float3     position     = hlslpp::float3(0.0f, 0.0f, 0.0f);
        hlslpp::quaternion rotation     = hlslpp::quaternion::identity();
        float              scale        = 1.0f;
        bool               initialized  = false;    // 現在の姿勢を Transform から取り込んだか

        // 放物線の移動（飛行中は目標への追従より優先する）
        bool               flying        = false;
        float              flightTime    = 0.0f;     // 経過時間（負の間は待ち）
        float              flightLength  = 0.4f;     // 所要時間（秒）
        float              flightArc     = 60.0f;    // 弧の高さ
        hlslpp::float3     flightFrom    = hlslpp::float3(0.0f, 0.0f, 0.0f);
        hlslpp::quaternion flightFromRot = hlslpp::quaternion::identity();
        float              flightFromScale = 1.0f;

        // 震え（間違えたときなど）
        float shakeTime      = 0.0f;    // 残り時間（秒）
        float shakeLength    = 0.0f;    // 全体の長さ（秒）
        float shakeAmplitude = 0.0f;    // 振れ幅

        // 弾み（置いたときなど）
        float punchTime   = 0.0f;    // 残り時間（秒）
        float punchLength = 0.0f;    // 全体の長さ（秒）
        float punchAmount = 0.0f;    // 拡大率に足す最大の量
    };

}    // namespace ChronoSort::ECS
