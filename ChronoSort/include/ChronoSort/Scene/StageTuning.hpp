//----------------------------------------------------------------------------
//! @file   StageTuning.hpp
//! @brief  卓のカメラと背景の机の調整値（ChronoSort/Assets/Data/Tuning/Stage.json）
//! @detail 対局のカメラの揺れと漂い（CameraRigSystem）と、タイトル・クレジットの机
//!         （散らかしたカードと見回すカメラ。DeskStage）です。
//!         メンバの初期値が JSON を書く前の値で、JSON に書いていない項目はこの値のままです。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Game/TuningTypes.hpp>

#include <hlsl++.h>

namespace ChronoSort::Scene {

    //! 卓のカメラと背景の机の調整値です。
    struct StageTuning {
        //----------------------------------------------------------------------------
        // 対局のカメラ（CameraRigSystem）
        //----------------------------------------------------------------------------
        float shakeDecay  = 18.0f;    // 揺れが1秒あたりに収まる量
        float shakeFreqX  = 71.0f;    // 揺れの左右の速さ
        float shakeFreqY  = 53.0f;    // 揺れの上下の速さ
        float shakeYScale = 0.6f;     // 上下の揺れの大きさ（左右に対して）
        float shakeLookAt = 0.5f;     // 注視点を揺らす割合
        float driftFreqX  = 0.23f;    // 漂いの左右の速さ
        float driftFreqY  = 0.31f;    // 漂いの上下の速さ
        float driftX      = 6.0f;     // 漂いの左右の幅
        float driftY      = 4.0f;     // 漂いの上下の幅

        //----------------------------------------------------------------------------
        // タイトル・クレジットの机（DeskStage）
        //----------------------------------------------------------------------------
        int   titleCards      = 14;        // タイトルで散らかすカードの枚数
        int   creditsCards    = 8;         // クレジットで散らかすカードの枚数
        float scatterHalfX    = 520.0f;    // 散らかす左右の範囲
        float scatterNearZ    = -120.0f;   // 散らかす奥行きの手前
        float scatterDepth    = 460.0f;    // 散らかす奥行きの幅
        float scatterStack    = 0.7f;      // 1枚ごとに積む高さ
        float scatterFaceUp   = 0.5f;      // 表向きにする割合
        float scatterYaw      = 0.35f;     // 向きのばらつき（半回転に対する割合）
        float dropSpreadX     = 0.6f;      // 舞い落ち始める左右の位置（置く位置に掛ける）
        float dropHeight      = 420.0f;    // 舞い落ち始める高さ
        float dropHeightRange = 200.0f;    // 舞い落ち始める高さのばらつき
        float dropBack        = 200.0f;    // 舞い落ち始める位置の奥へのずれ
        float dropSeconds     = 0.9f;      // 舞い落ちる時間
        float dropSecondsRange = 0.4f;     // 舞い落ちる時間のばらつき
        float dropArc         = 40.0f;     // 舞い落ちる弧の高さ
        float dropDelay       = 0.1f;      // 1枚目が落ち始めるまで
        float dropStagger     = 0.07f;     // 1枚ごとのずらし

        float orbitSpeed       = 0.07f;    // 見回す速さ
        float orbitAngle       = 0.35f;    // 見回す角度（ラジアン）
        float orbitRadius      = 520.0f;   // 見回す半径
        float orbitCenterZ     = 80.0f;    // 見回す中心の奥行き
        float orbitHeight      = 430.0f;   // カメラの高さ
        float orbitBobSpeed    = 0.11f;    // 高さの上下の速さ
        float orbitBob         = 25.0f;    // 高さの上下の幅
        float lookAtSwaySpeed  = 0.05f;    // 注視点の左右の速さ
        float lookAtSway       = 60.0f;    // 注視点の左右の幅
        float lookAtZ          = 90.0f;    // 注視点の奥行き
    };

    //! StageTuning を読み込みます（書いていない項目は既定値のまま）。
    template <class Archive>
    void load(Archive& archive, StageTuning& t) {
#define CHRONOSORT_LOAD(name) Tsukino::IO::LoadOptional(archive, #name, t.name)
        CHRONOSORT_LOAD(shakeDecay);
        CHRONOSORT_LOAD(shakeFreqX);
        CHRONOSORT_LOAD(shakeFreqY);
        CHRONOSORT_LOAD(shakeYScale);
        CHRONOSORT_LOAD(shakeLookAt);
        CHRONOSORT_LOAD(driftFreqX);
        CHRONOSORT_LOAD(driftFreqY);
        CHRONOSORT_LOAD(driftX);
        CHRONOSORT_LOAD(driftY);
        CHRONOSORT_LOAD(titleCards);
        CHRONOSORT_LOAD(creditsCards);
        CHRONOSORT_LOAD(scatterHalfX);
        CHRONOSORT_LOAD(scatterNearZ);
        CHRONOSORT_LOAD(scatterDepth);
        CHRONOSORT_LOAD(scatterStack);
        CHRONOSORT_LOAD(scatterFaceUp);
        CHRONOSORT_LOAD(scatterYaw);
        CHRONOSORT_LOAD(dropSpreadX);
        CHRONOSORT_LOAD(dropHeight);
        CHRONOSORT_LOAD(dropHeightRange);
        CHRONOSORT_LOAD(dropBack);
        CHRONOSORT_LOAD(dropSeconds);
        CHRONOSORT_LOAD(dropSecondsRange);
        CHRONOSORT_LOAD(dropArc);
        CHRONOSORT_LOAD(dropDelay);
        CHRONOSORT_LOAD(dropStagger);
        CHRONOSORT_LOAD(orbitSpeed);
        CHRONOSORT_LOAD(orbitAngle);
        CHRONOSORT_LOAD(orbitRadius);
        CHRONOSORT_LOAD(orbitCenterZ);
        CHRONOSORT_LOAD(orbitHeight);
        CHRONOSORT_LOAD(orbitBobSpeed);
        CHRONOSORT_LOAD(orbitBob);
        CHRONOSORT_LOAD(lookAtSwaySpeed);
        CHRONOSORT_LOAD(lookAtSway);
        CHRONOSORT_LOAD(lookAtZ);
#undef CHRONOSORT_LOAD
    }

}    // namespace ChronoSort::Scene
