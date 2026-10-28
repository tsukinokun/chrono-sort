//----------------------------------------------------------------------------
//! @file   SaveData.hpp
//! @brief  記録（ソロの最高得点など）の保存
//! @detail ワークスペースルート（配布時は exe の隣）の Saves/ChronoSort.json に保存します。
//!         ソロの記録は出題の範囲ごとに分けて持ちます。範囲を分ける前の形（version 1）の
//!         ファイルも読めて、その記録は通史の記録として引き継ぎます。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/Deck.hpp>

#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>

#include <string>

namespace ChronoSort::Game {

    //! ソロの1つの範囲の記録です。
    struct SoloRecord {
        int bestScore  = 0;    // 最高得点
        int bestPlaced = 0;    // 並べた最多枚数
        int bestStreak = 0;    // 最長の連続正解

        //! 何か記録があるかを返します。
        //! @return 記録があれば true。
        bool HasAny() const { return bestScore > 0 || bestPlaced > 0 || bestStreak > 0; }

        template <class Archive>
        void serialize(Archive& archive) {
            archive(cereal::make_nvp("bestScore", bestScore),
                    cereal::make_nvp("bestPlaced", bestPlaced),
                    cereal::make_nvp("bestStreak", bestStreak));
        }
    };

    //! 保存する記録です。
    struct SaveData {
        int         version    = 2;        // 保存の形の版
        SoloRecord  all;                   // 通史のソロの記録
        SoloRecord  japan;                 // 日本史だけのソロの記録
        SoloRecord  world;                 // 世界史だけのソロの記録
        int         versusWins = 0;        // CPU 対戦の勝ち数（範囲で分けない）
        std::string range      = "all";    // 最後に選んだ出題の範囲（Rules::RangeKey の値）

        //! 範囲のソロの記録を返します。
        //! @param  [in] range 出題の範囲。
        //! @return 記録。
        SoloRecord& Record(Rules::CardRange range) {
            return range == Rules::CardRange::Japan ? japan : (range == Rules::CardRange::World ? world : all);
        }

        //! 範囲のソロの記録を返します。
        //! @param  [in] range 出題の範囲。
        //! @return 記録。
        const SoloRecord& Record(Rules::CardRange range) const {
            return range == Rules::CardRange::Japan ? japan : (range == Rules::CardRange::World ? world : all);
        }

        template <class Archive>
        void serialize(Archive& archive) {
            archive(cereal::make_nvp("version", version),
                    cereal::make_nvp("all", all),
                    cereal::make_nvp("japan", japan),
                    cereal::make_nvp("world", world),
                    cereal::make_nvp("versusWins", versusWins),
                    cereal::make_nvp("range", range));
        }
    };

    //! 記録を読み込みます。ファイルが無ければ初期値を返します。
    //! @return 記録。
    SaveData LoadSaveData();

    //! 記録を書き込みます。
    //! @param  [in] data 記録。
    void StoreSaveData(const SaveData& data);

}    // namespace ChronoSort::Game
