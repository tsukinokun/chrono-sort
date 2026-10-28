//----------------------------------------------------------------------------
//! @file   BoardTuning.hpp
//! @brief  卓の上の並べ方の調整値（ChronoSort/Assets/Data/Tuning/Board.json）
//! @detail 場の列・手札・CPU の席・山札・捨て札の置き方と、指したときの起こし方です。
//!         メンバの初期値が JSON を書く前の値で、JSON に書いていない項目はこの値のままです。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Game/TuningTypes.hpp>

#include <hlsl++.h>

#include <cstddef>

namespace ChronoSort::Board {

    //! 卓の上の並べ方の調整値です。
    struct BoardTuning {
        //----------------------------------------------------------------------------
        // 場の列
        //----------------------------------------------------------------------------
        float rowZ        = 70.0f;     // 場の列の奥行き
        float dragHeight  = 45.0f;     // ドラッグ中のカードの高さ
        float rowTilt     = 0.45f;     // 手前の辺を卓に付けたままカメラへ起こす角度（ラジアン。真正面は約 0.75）
        float rowHalfSpan = 430.0f;    // カードの中心を置いてよい左右の範囲（画面の端で切れない幅）

        // 少ないうちは全部を大きく並べ、増えたら注目点の周りだけ大きく広げて遠いカードほど細く詰める
        float rowScaleMax   = 1.4f;     // 通常の拡大率（注目点の周りもこの大きさ）
        float rowScaleFar   = 0.9f;     // 注目点から遠いカードの拡大率の下限
        float rowSpacingMax = 74.0f;    // 拡大率 1 のときの、重ならない間隔
        float narrowMin     = 14.0f;    // 遠いカードを詰める間隔の下限（縁が覗く程度）
        float focusRadius   = 2.2f;     // 注目点の周りで広げる範囲（枚数）
        float gapWidth      = 60.0f;    // 挿入位置の隙間の広さ（拡大率 1 のとき）
        float fitSharpness  = 6.0f;     // 拡大率・間隔・流す量の追従の速さ
        float gapSharpness  = 14.0f;    // 隙間が開閉する速さ
        float scrollToSlot  = 0.6f;     // スロットがこの割合（rowHalfSpan に対して）より外なら、見えるところまで流す

        // 範囲の外のカードは端に畳んで重ねる
        float foldDepth   = 26.0f;    // 畳んで重ねる奥行き
        float foldFalloff = 4.0f;     // 遠いほど詰まる度合い（foldDepth の何倍の距離で詰まりきるか）
        float foldShrink  = 0.15f;    // いちばん奥で小さくする割合
        float foldSink    = 14.0f;    // いちばん奥で奥へ下げる量

        float rowDepthJitter = 6.0f;     // 並びの前後のばらつき
        float rowFocusPull   = 12.0f;    // 注目点に近いカードを手前に出す量
        float rowYawJitter   = 0.06f;    // 並びの向きのばらつき（ラジアン）

        // 指したカードは手前に起こして大きくし、年号を読みやすくする
        float rowHoverTilt    = 0.55f;    // 起こす角度（ラジアン）
        float rowHoverHeight  = 60.0f;    // 浮かせる高さ
        float rowHoverForward = 60.0f;    // 手前へ出す量
        float rowHoverScale   = 1.25f;    // 拡大率に掛ける倍率

        float markerSink = 0.15f;    // 挿入位置の目印を卓へ沈める量

        //----------------------------------------------------------------------------
        // 人間の手札（手前の扇）
        //----------------------------------------------------------------------------
        float handCenterZ   = -230.0f;
        float handBaseY     = 90.0f;
        float handMaxSpread = 330.0f;    // 扇の全体の幅の上限
        float handSpacing   = 56.0f;     // 枚数が少ないときの間隔
        float handTilt      = -0.55f;    // カメラへ向ける傾き（X 軸まわり）
        float handFanAngle  = 0.07f;     // 1枚あたりの扇の開き
        float handArcDepth  = 6.0f;      // 端ほど奥へ下げる量
        float handStackStep = 1.5f;      // 1枚ごとに積む高さ
        float hoverAvoid    = 22.0f;     // ホバー中のカードの両隣が避ける量

        // ホバー：手前に浮かせて拡大し、カーソルの位置へ少し傾ける
        hlslpp::float3 handHoverOffset = hlslpp::float3(0.0f, 30.0f, -28.0f);
        float          handHoverLean   = 0.35f;    // カーソルの位置へ傾ける量
        float          handHoverTilt   = -0.2f;    // さらにカメラへ向ける傾き
        float          handHoverScale  = 1.22f;

        // 選択中：手札の上に掲げて、ゆっくり上下させる
        float liftedSpread       = 0.4f;     // 並びの位置に掛ける倍率（中央へ寄せる）
        float liftedHeight       = 70.0f;
        float liftedForward      = 70.0f;
        float liftedBobFrequency = 2.4f;
        float liftedBobAmplitude = 3.0f;
        float liftedTilt         = -0.75f;
        float liftedScale        = 1.15f;

        //----------------------------------------------------------------------------
        // CPU の席（卓の奥）
        //----------------------------------------------------------------------------
        float seatZ         = 300.0f;    // 席の奥行き
        float seatSpread    = 320.0f;    // 席を並べる左右の幅（CPU が2人以上のとき）
        float seatPullBack  = 30.0f;     // 端の席ほど手前へ寄せる量
        float seatScale     = 0.72f;
        float seatSpacing   = 34.0f;
        float seatStackStep = 0.7f;
        float seatFanYaw    = 0.06f;
        float seatYawJitter = 0.08f;

        // 思考中：手に取ってカメラへ傾け、迷うように揺らす
        hlslpp::float3 seatLiftedOffset    = hlslpp::float3(0.0f, 55.0f, -40.0f);
        float          seatSwayFrequency   = 4.0f;
        float          seatSwayAmplitude   = 0.06f;
        float          seatLiftedTilt      = -0.6f;
        float          seatLiftedScale     = 0.95f;

        //----------------------------------------------------------------------------
        // 山札・捨て札。場の列の両端に置くと長くなった列とぶつかるため、列より手前の左右の角に置く
        //----------------------------------------------------------------------------
        hlslpp::float3 deckPosition    = hlslpp::float3(350.0f, 0.0f, -120.0f);
        hlslpp::float3 discardPosition = hlslpp::float3(-350.0f, 0.0f, -115.0f);
        float          coverThickness  = 1.4f;    // 山札の裏1枚分の見た目の厚み
        int            maxCovers       = 9;       // 山札の裏の枚数の上限
        int            cardsPerCover   = 5;       // 残り何枚ごとに裏を1枚置くか
        float          coverJitter     = 1.5f;    // 裏の位置のばらつき
        float          coverYawJitter  = 0.05f;   // 裏の向きのばらつき
        float          deckTopLift     = 1.0f;    // 山札のいちばん上から引くカードの浮かせ方

        float          discardStackStep = 0.8f;
        float          discardJitter    = 14.0f;    // 捨て札の位置のばらつき
        float          discardYawJitter = 0.35f;    // 捨て札の向きのばらつき
        Game::Flight   discardFlight    = {0.55f, 90.0f, 0.0f};

        // 挿入位置の目印の光り方（脈打ち）。warning は不正解の後に正しい位置を示すとき
        Game::GlowSpec markerGlow        = {hlslpp::float3(0.9f, 0.65f, 0.22f), 1.0f, 6.0f};
        Game::GlowSpec markerWarningGlow = {hlslpp::float3(1.2f, 0.12f, 0.08f), 1.0f, 6.0f};
    };

    //! BoardTuning を読み込みます（書いていない項目は既定値のまま）。
    template <class Archive>
    void load(Archive& archive, BoardTuning& t) {
#define CHRONOSORT_LOAD(name) Tsukino::IO::LoadOptional(archive, #name, t.name)
        CHRONOSORT_LOAD(rowZ);
        CHRONOSORT_LOAD(dragHeight);
        CHRONOSORT_LOAD(rowTilt);
        CHRONOSORT_LOAD(rowHalfSpan);
        CHRONOSORT_LOAD(rowScaleMax);
        CHRONOSORT_LOAD(rowScaleFar);
        CHRONOSORT_LOAD(rowSpacingMax);
        CHRONOSORT_LOAD(narrowMin);
        CHRONOSORT_LOAD(focusRadius);
        CHRONOSORT_LOAD(gapWidth);
        CHRONOSORT_LOAD(fitSharpness);
        CHRONOSORT_LOAD(gapSharpness);
        CHRONOSORT_LOAD(scrollToSlot);
        CHRONOSORT_LOAD(foldDepth);
        CHRONOSORT_LOAD(foldFalloff);
        CHRONOSORT_LOAD(foldShrink);
        CHRONOSORT_LOAD(foldSink);
        CHRONOSORT_LOAD(rowDepthJitter);
        CHRONOSORT_LOAD(rowFocusPull);
        CHRONOSORT_LOAD(rowYawJitter);
        CHRONOSORT_LOAD(rowHoverTilt);
        CHRONOSORT_LOAD(rowHoverHeight);
        CHRONOSORT_LOAD(rowHoverForward);
        CHRONOSORT_LOAD(rowHoverScale);
        CHRONOSORT_LOAD(markerSink);
        CHRONOSORT_LOAD(handCenterZ);
        CHRONOSORT_LOAD(handBaseY);
        CHRONOSORT_LOAD(handMaxSpread);
        CHRONOSORT_LOAD(handSpacing);
        CHRONOSORT_LOAD(handTilt);
        CHRONOSORT_LOAD(handFanAngle);
        CHRONOSORT_LOAD(handArcDepth);
        CHRONOSORT_LOAD(handStackStep);
        CHRONOSORT_LOAD(hoverAvoid);
        CHRONOSORT_LOAD(handHoverOffset);
        CHRONOSORT_LOAD(handHoverLean);
        CHRONOSORT_LOAD(handHoverTilt);
        CHRONOSORT_LOAD(handHoverScale);
        CHRONOSORT_LOAD(liftedSpread);
        CHRONOSORT_LOAD(liftedHeight);
        CHRONOSORT_LOAD(liftedForward);
        CHRONOSORT_LOAD(liftedBobFrequency);
        CHRONOSORT_LOAD(liftedBobAmplitude);
        CHRONOSORT_LOAD(liftedTilt);
        CHRONOSORT_LOAD(liftedScale);
        CHRONOSORT_LOAD(seatZ);
        CHRONOSORT_LOAD(seatSpread);
        CHRONOSORT_LOAD(seatPullBack);
        CHRONOSORT_LOAD(seatScale);
        CHRONOSORT_LOAD(seatSpacing);
        CHRONOSORT_LOAD(seatStackStep);
        CHRONOSORT_LOAD(seatFanYaw);
        CHRONOSORT_LOAD(seatYawJitter);
        CHRONOSORT_LOAD(seatLiftedOffset);
        CHRONOSORT_LOAD(seatSwayFrequency);
        CHRONOSORT_LOAD(seatSwayAmplitude);
        CHRONOSORT_LOAD(seatLiftedTilt);
        CHRONOSORT_LOAD(seatLiftedScale);
        CHRONOSORT_LOAD(deckPosition);
        CHRONOSORT_LOAD(discardPosition);
        CHRONOSORT_LOAD(coverThickness);
        CHRONOSORT_LOAD(maxCovers);
        CHRONOSORT_LOAD(cardsPerCover);
        CHRONOSORT_LOAD(coverJitter);
        CHRONOSORT_LOAD(coverYawJitter);
        CHRONOSORT_LOAD(deckTopLift);
        CHRONOSORT_LOAD(discardStackStep);
        CHRONOSORT_LOAD(discardJitter);
        CHRONOSORT_LOAD(discardYawJitter);
        CHRONOSORT_LOAD(discardFlight);
        CHRONOSORT_LOAD(markerGlow);
        CHRONOSORT_LOAD(markerWarningGlow);
#undef CHRONOSORT_LOAD
    }

}    // namespace ChronoSort::Board
