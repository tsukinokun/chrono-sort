//----------------------------------------------------------------------------
//! @file   GameRules.hpp
//! @brief  Timeline 準拠のルール定数
//----------------------------------------------------------------------------
#pragma once

namespace ChronoSort::Rules {

    constexpr int kMinPlayers    = 2;    // 対戦の最少人数
    constexpr int kMaxPlayers    = 8;    // 対戦の最大人数
    constexpr int kSoloLifeCount = 3;    // ソロで間違えられる回数

    //! 人数に応じた初期手札の枚数を返します。
    //! 2〜3人は6枚、4〜5人は5枚、6〜8人は4枚です。
    //! @param  [in] playerCount プレイ人数。
    //! @return 初期手札の枚数。
    constexpr int GetInitialHandSize(int playerCount) {
        if(playerCount <= 3) {
            return 6;
        }
        if(playerCount <= 5) {
            return 5;
        }
        return 4;
    }

}    // namespace ChronoSort::Rules
