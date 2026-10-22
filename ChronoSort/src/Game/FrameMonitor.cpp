//----------------------------------------------------------------------------
//! @file   FrameMonitor.cpp
//! @brief  引っかかり（長いフレーム）の記録の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Game/FrameMonitor.hpp>

#include <Tsukino/Core/Log.hpp>

#include <algorithm>
#include <cstdio>

namespace ChronoSort::Game::FrameMonitor {

    namespace {
        std::string g_lastMark  = "start";    // 直前の出来事
        int         g_hitches   = 0;          // 引っかかりの回数
        float       g_worst     = 0.0f;       // 最長のフレーム（秒）
        int         g_frames    = 0;          // 数えたフレーム数
    }    // namespace

    //----------------------------------------------------------------------------
    //! 直前の出来事を記録します。
    //----------------------------------------------------------------------------
    void Mark(const std::string& label) {
        g_lastMark = label;
    }

    //----------------------------------------------------------------------------
    //! 1フレームの長さを報告します。
    //----------------------------------------------------------------------------
    void Tick(float seconds) {
        ++g_frames;
        g_worst = std::max(g_worst, seconds);
        if(seconds < kHitchSeconds) {
            return;
        }
        ++g_hitches;
        char line[160];
        std::snprintf(line, sizeof(line), "[Hitch] %.1fms after %s", seconds * 1000.0f, g_lastMark.c_str());
        Tsukino::Core::Log::Warn(line);
    }

    //----------------------------------------------------------------------------
    //! 引っかかりの集計をログへ出し、数え直します。
    //----------------------------------------------------------------------------
    void Report() {
        char line[160];
        std::snprintf(line, sizeof(line), "[Hitch] summary: %d hitches in %d frames, worst %.1fms", g_hitches, g_frames, g_worst * 1000.0f);
        Tsukino::Core::Log::Info(line);
        g_hitches = 0;
        g_frames  = 0;
        g_worst   = 0.0f;
    }

}    // namespace ChronoSort::Game::FrameMonitor
