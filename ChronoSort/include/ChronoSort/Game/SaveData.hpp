//----------------------------------------------------------------------------
//! @file   SaveData.hpp
//! @brief  記録（ソロの最高得点など）の保存
//! @detail ワークスペースルート（配布時は exe の隣）の Saves/ChronoSort.json に保存します。
//!         ソロの記録は出題の範囲ごとに分けて持ちます。範囲を分ける前の形（version 1）の
//!         ファイルも読めて、その記録は通史の記録として引き継ぎます。
//!         version 3 からは出題の難易度ごとにも分けます（version 2 までの記録は、むずかしいの記録になる）。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/Deck.hpp>

#include <Tsukino/Core/IO/CerealOptional.hpp>

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
    //! @note   難易度を分ける前（version 2 まで）の all・japan・world は全部のカードで遊んだ記録なので、
    //!         そのまま「むずかしい」の記録として使う。やさしい・ふつうの記録は別の項目に持つ
    struct SaveData {
        int         version    = 3;         // 保存の形の版
        SoloRecord  all;                    // 通史・むずかしいのソロの記録
        SoloRecord  japan;                  // 日本史・むずかしいのソロの記録
        SoloRecord  world;                  // 世界史・むずかしいのソロの記録
        SoloRecord  allEasy;                // 通史・やさしい
        SoloRecord  japanEasy;              // 日本史・やさしい
        SoloRecord  worldEasy;              // 世界史・やさしい
        SoloRecord  allNormal;              // 通史・ふつう
        SoloRecord  japanNormal;            // 日本史・ふつう
        SoloRecord  worldNormal;            // 世界史・ふつう
        int         versusWins = 0;         // CPU 対戦の勝ち数（範囲で分けない）
        std::string range      = "all";     // 最後に選んだ出題の範囲（Rules::RangeKey の値）
        std::string difficulty = "hard";    // 最後に選んだ出題の難易度（Rules::DifficultyKey の値）
        int         tutorialMatches = 0;    // 遊び方の説明つきで、カードを1枚以上正しく置けた対局の数

        //! 範囲と難易度のソロの記録を返します。
        //! @param  [in] range      出題の範囲。
        //! @param  [in] difficulty 出題の難易度。
        //! @return 記録。
        SoloRecord& Record(Rules::CardRange range, Rules::CardDifficulty difficulty) {
            return const_cast<SoloRecord&>(static_cast<const SaveData&>(*this).Record(range, difficulty));
        }

        //! 範囲と難易度のソロの記録を返します。
        //! @param  [in] range      出題の範囲。
        //! @param  [in] difficulty 出題の難易度。
        //! @return 記録。
        const SoloRecord& Record(Rules::CardRange range, Rules::CardDifficulty difficulty) const {
            using Rules::CardDifficulty;
            using Rules::CardRange;
            if(difficulty == CardDifficulty::Easy) {
                return range == CardRange::Japan ? japanEasy : (range == CardRange::World ? worldEasy : allEasy);
            }
            if(difficulty == CardDifficulty::Normal) {
                return range == CardRange::Japan ? japanNormal : (range == CardRange::World ? worldNormal : allNormal);
            }
            return range == CardRange::Japan ? japan : (range == CardRange::World ? world : all);
        }

        template <class Archive>
        void save(Archive& archive) const {
            archive(cereal::make_nvp("version", version),
                    cereal::make_nvp("all", all),
                    cereal::make_nvp("japan", japan),
                    cereal::make_nvp("world", world),
                    cereal::make_nvp("allEasy", allEasy),
                    cereal::make_nvp("japanEasy", japanEasy),
                    cereal::make_nvp("worldEasy", worldEasy),
                    cereal::make_nvp("allNormal", allNormal),
                    cereal::make_nvp("japanNormal", japanNormal),
                    cereal::make_nvp("worldNormal", worldNormal),
                    cereal::make_nvp("versusWins", versusWins),
                    cereal::make_nvp("range", range),
                    cereal::make_nvp("difficulty", difficulty),
                    cereal::make_nvp("tutorialMatches", tutorialMatches));
        }

        //! 読み込みは、あとから足した項目（難易度ごとの記録・difficulty・tutorialMatches）が無い記録も読めるようにする（無ければ初期値のまま）。
        template <class Archive>
        void load(Archive& archive) {
            archive(cereal::make_nvp("version", version),
                    cereal::make_nvp("all", all),
                    cereal::make_nvp("japan", japan),
                    cereal::make_nvp("world", world),
                    cereal::make_nvp("versusWins", versusWins),
                    cereal::make_nvp("range", range));
            Tsukino::IO::LoadOptional(archive, "allEasy", allEasy);
            Tsukino::IO::LoadOptional(archive, "japanEasy", japanEasy);
            Tsukino::IO::LoadOptional(archive, "worldEasy", worldEasy);
            Tsukino::IO::LoadOptional(archive, "allNormal", allNormal);
            Tsukino::IO::LoadOptional(archive, "japanNormal", japanNormal);
            Tsukino::IO::LoadOptional(archive, "worldNormal", worldNormal);
            Tsukino::IO::LoadOptional(archive, "difficulty", difficulty);
            Tsukino::IO::LoadOptional(archive, "tutorialMatches", tutorialMatches);
        }
    };

    //! 記録を読み込みます。ファイルが無ければ初期値を返します。
    //! @return 記録。
    SaveData LoadSaveData();

    //! 記録を書き込みます。
    //! @param  [in] data 記録。
    void StoreSaveData(const SaveData& data);

}    // namespace ChronoSort::Game
