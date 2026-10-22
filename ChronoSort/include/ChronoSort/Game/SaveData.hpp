//----------------------------------------------------------------------------
//! @file   SaveData.hpp
//! @brief  記録（ソロの最高得点など）の保存
//! @detail ワークスペースルート（配布時は exe の隣）の Saves/ChronoSort.json に保存します。
//----------------------------------------------------------------------------
#pragma once
#include <cereal/cereal.hpp>

namespace ChronoSort::Game {

    //! 保存する記録です。
    struct SaveData {
        int bestScore  = 0;    // ソロの最高得点
        int bestPlaced = 0;    // ソロで並べた最多枚数
        int bestStreak = 0;    // 最長の連続正解
        int versusWins = 0;    // CPU 対戦の勝ち数

        template <class Archive>
        void serialize(Archive& archive) {
            archive(cereal::make_nvp("bestScore", bestScore),
                    cereal::make_nvp("bestPlaced", bestPlaced),
                    cereal::make_nvp("bestStreak", bestStreak),
                    cereal::make_nvp("versusWins", versusWins));
        }
    };

    //! 記録を読み込みます。ファイルが無ければ初期値を返します。
    //! @return 記録。
    SaveData LoadSaveData();

    //! 記録を書き込みます。
    //! @param  [in] data 記録。
    void StoreSaveData(const SaveData& data);

}    // namespace ChronoSort::Game
