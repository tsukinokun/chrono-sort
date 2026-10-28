//----------------------------------------------------------------------------
//! @file   CountUpTextComponent.hpp
//! @brief  数字の文字を目標の値まで数え上げる印
//! @detail 得点の封蝋や結果の紙の得点など、値が増えたときに一気に変えずに数え上げて見せる文字に付けます。
//!         CountUpTextSystem が shown を進め、整数が変わったときだけ FontComponent の文字を書き換えます。
//----------------------------------------------------------------------------
#pragma once

namespace ChronoSort::ECS {

    //! 数字の文字を目標の値まで数え上げる印です。
    //! @note   duration が 0 なら、target を追いかける（離れているほど速く、最低でも minSpeed）。
    //!         duration が正なら、delay のあと duration 秒で 0 から target まで、終わりほどゆっくり数え上げる。
    struct CountUpTextComponent {
        float target   = 0.0f;    // 目標の値
        float shown    = 0.0f;    // 今見せている値
        float minSpeed = 8.0f;    // 追いかけるときの最低の速さ（1秒あたり）
        float chase    = 6.0f;    // 追いかけるときの速さ（残りの何倍を1秒で進むか）
        float delay    = 0.0f;    // 時間で数え上げるときの待ち（秒）
        float duration = 0.0f;    // 時間で数え上げるときの長さ（秒。0 なら追いかける）
        float elapsed  = 0.0f;    // 時間で数え上げ始めてからの時間（秒）
        long long written = -1;   // 最後に書いた整数（変わったときだけ書き換える）
    };

}    // namespace ChronoSort::ECS
