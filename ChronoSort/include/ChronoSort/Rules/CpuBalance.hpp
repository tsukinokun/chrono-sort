//----------------------------------------------------------------------------
//! @file   CpuBalance.hpp
//! @brief  対戦 CPU の強さの測定
//! @detail 人の代わりの打ち手と CPU 1人の対戦を、画面を出さずに何度も回し、
//!         段ごとの勝ち負けをログへ出します。CPU の調整値を詰めるときの確認用です。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/CardData.hpp>
#include <ChronoSort/Rules/RulesTuning.hpp>

#include <vector>

namespace ChronoSort::Rules {

    //! CPU の段ごとに、人の代わりの打ち手との対戦を回して結果をログへ出します。
    //! @param  [in] rules ルールの調整値（CPU の強さ）。
    //! @param  [in] cards 使うカードの一覧。
    //! @note   人の代わりは、決まった正解率で正しい場所に置き、外すときは正解の隣に置く。
    //!         正解率を何通りか変えて測り、人の上手さで結果がどう動くかも出す
    void MeasureCpuBalance(const RulesTuning& rules, const std::vector<CardData>& cards);

}    // namespace ChronoSort::Rules
