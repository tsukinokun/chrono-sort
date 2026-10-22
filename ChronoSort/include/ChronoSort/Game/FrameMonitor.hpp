//----------------------------------------------------------------------------
//! @file   FrameMonitor.hpp
//! @brief  引っかかり（長いフレーム）の記録
//! @detail 1フレームが閾値を超えたら、直前に起きた出来事と一緒にログへ出します。
//!         「配った直後に 80ms 止まった」のように、どの操作が重いかを後から追えるようにするためです。
//----------------------------------------------------------------------------
#pragma once
#include <string>

namespace ChronoSort::Game::FrameMonitor {

    constexpr float kHitchSeconds = 0.034f;    // これより長いフレームを引っかかりとして記録する（60fps で2フレーム落ち）

    //! 直前の出来事を記録します（引っかかりのログに添える）。
    //! @param  [in] label 出来事の名前（例："deal"）。
    void Mark(const std::string& label);

    //! 1フレームの長さを報告します。閾値を超えていればログへ出します。
    //! @param  [in] seconds フレームの長さ（秒）。
    void Tick(float seconds);

    //! 引っかかりの集計をログへ出し、数え直します。
    void Report();

}    // namespace ChronoSort::Game::FrameMonitor
