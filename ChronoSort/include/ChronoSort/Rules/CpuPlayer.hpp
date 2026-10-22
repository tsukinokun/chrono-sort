//----------------------------------------------------------------------------
//! @file   CpuPlayer.hpp
//! @brief  CPU の手の選び方
//! @detail CPU は正解を「知って」いますが、人間らしく迷わせます。
//!         年代が隣と近いカードほど外しやすく、外すときは正解の近くに置きます。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/Match.hpp>

#include <random>

namespace ChronoSort::Rules {

    //! CPU が選んだ手です。
    struct CpuMove {
        size_t handIndex  = 0;        // 出す手札の番号
        size_t slot       = 0;        // 置くスロット
        float  confidence = 0.0f;     // 自信の度合い（0〜1）。考える時間の演出に使う
    };

    //! 手札から1枚選び、置くスロットを決めます。
    //! @param  [in]     player   手番の CPU。
    //! @param  [in]     timeline 場の列。
    //! @param  [in,out] rng      乱数生成器。
    //! @return 選んだ手。手札が空なら handIndex は 0 のまま。
    CpuMove ChooseCpuMove(const Player& player, const Timeline& timeline, std::mt19937& rng);

    //! カードを正しい位置に置ける確率を返します。
    //! 隣のカードとの年代差が小さいほど低くなります。
    //! @param  [in] level    CPU の強さ。
    //! @param  [in] timeline 場の列。
    //! @param  [in] card     置くカード。
    //! @return 正解する確率（0〜1）。
    float CpuSuccessChance(CpuLevel level, const Timeline& timeline, const CardData& card);

}    // namespace ChronoSort::Rules
