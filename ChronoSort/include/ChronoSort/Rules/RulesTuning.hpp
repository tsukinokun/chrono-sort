//----------------------------------------------------------------------------
//! @file   RulesTuning.hpp
//! @brief  ルールの調整値（ChronoSort/Assets/Data/Tuning/Rules.json）
//! @detail 手札の枚数・ライフ・得点・CPU の強さと人物です。Match と CPU は MatchConfig::rules で受け取ります。
//!         メンバの初期値が JSON を書く前の値で、JSON に書いていない項目はこの値のままです。
//!         自己テスト（RulesSelfTest）はこの初期値で確かめます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/IO/CerealOptional.hpp>

#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace ChronoSort::Rules {

    //! 人数ごとの初期手札の枚数の段です（maxPlayers 人以下なら handSize 枚）。
    struct HandSizeStep {
        int maxPlayers = 8;    // この人数まで
        int handSize   = 4;    // 初期手札の枚数
    };

    //! CPU の強さ1段ぶんです。
    struct CpuLevelRules {
        float hardestChance = 0.55f;    // 隣との年代差がとても近いときの正解率
        float easiestChance = 0.92f;    // 隣との年代差が十分離れているときの正解率
        float whim          = 0.25f;    // 自信のあるカードではなく気まぐれに選ぶ割合
    };

    //! 対戦の CPU 1人の人物です。
    struct CpuProfile {
        std::string name;        // 名前（UTF-8）
        std::string portrait;    // 肖像の画像（ChronoSort/Assets/Textures/UI/ からのパス。ルールは使わず、画面に渡すだけ）
    };

    //! ルールの調整値です。
    struct RulesTuning {
        int soloHandSize   = 3;      // ソロで保つ手札の枚数
        int soloLives      = 3;      // ソロで間違えられる回数
        int soloBaseScore  = 100;    // 1枚正解の基本点
        int closeCallBonus = 50;     // 隣と年代が近いときの加点
        int closeCallYears = 20;     // 「近い」とみなす年数
        int streakCap      = 10;     // 倍率（1割ずつ）が伸びる連続正解数の上限
        int rankS          = 30;     // ソロの結果で S になる並べた枚数（出し切っても S）
        int rankA          = 20;     // A になる並べた枚数
        int rankB          = 10;     // B になる並べた枚数（届かなければ C）

        std::vector<HandSizeStep> handSizes = {{3, 6}, {5, 5}, {8, 4}};    // 対戦の人数ごとの初期手札

        //! CPU の強さ（CpuLevel の Easy・Normal・Hard の順）
        std::vector<CpuLevelRules> cpuLevels = {{0.35f, 0.80f, 0.6f}, {0.55f, 0.92f, 0.25f}, {0.72f, 0.98f, 0.05f}};
        float comfortableGapYears = 150.0f;    // これ以上離れていれば CPU は迷わない年数
        float nearMissChance      = 0.8f;      // CPU が外すとき、正解のすぐ隣を選ぶ割合（残りは2つ隣）

        //! 対戦の CPU の人物（席の順）
        std::vector<CpuProfile> cpus = {
            {"ヘロドトス", "portrait_herodotus.png"},
            {"司馬遷", "portrait_sima_qian.png"},
            {"ギボン", "portrait_gibbon.png"},
        };

        //! 人数に応じた初期手札の枚数を返します。
        //! @param  [in] playerCount プレイ人数。
        //! @return 初期手札の枚数。
        int InitialHandSize(int playerCount) const {
            for(const HandSizeStep& step : handSizes) {
                if(playerCount <= step.maxPlayers) {
                    return step.handSize;
                }
            }
            return handSizes.empty() ? 4 : handSizes.back().handSize;
        }

        //! CPU の強さを返します（足りなければ最後の段）。
        //! @param  [in] index 強さの段（CpuLevel を数にしたもの）。
        //! @return 強さの調整値。
        const CpuLevelRules& Level(size_t index) const {
            static const CpuLevelRules kFallback;
            if(cpuLevels.empty()) {
                return kFallback;
            }
            return cpuLevels[index < cpuLevels.size() ? index : cpuLevels.size() - 1];
        }
    };

    //! HandSizeStep を読み込みます。
    template <class Archive>
    void load(Archive& archive, HandSizeStep& t) {
        Tsukino::IO::LoadOptional(archive, "maxPlayers", t.maxPlayers);
        Tsukino::IO::LoadOptional(archive, "handSize", t.handSize);
    }

    //! CpuLevelRules を読み込みます。
    template <class Archive>
    void load(Archive& archive, CpuLevelRules& t) {
        Tsukino::IO::LoadOptional(archive, "hardestChance", t.hardestChance);
        Tsukino::IO::LoadOptional(archive, "easiestChance", t.easiestChance);
        Tsukino::IO::LoadOptional(archive, "whim", t.whim);
    }

    //! CpuProfile を読み込みます。
    template <class Archive>
    void load(Archive& archive, CpuProfile& t) {
        Tsukino::IO::LoadOptional(archive, "name", t.name);
        Tsukino::IO::LoadOptional(archive, "portrait", t.portrait);
    }

    //! RulesTuning を読み込みます。
    template <class Archive>
    void load(Archive& archive, RulesTuning& t) {
#define CHRONOSORT_LOAD(name) Tsukino::IO::LoadOptional(archive, #name, t.name)
        CHRONOSORT_LOAD(soloHandSize);
        CHRONOSORT_LOAD(soloLives);
        CHRONOSORT_LOAD(soloBaseScore);
        CHRONOSORT_LOAD(closeCallBonus);
        CHRONOSORT_LOAD(closeCallYears);
        CHRONOSORT_LOAD(streakCap);
        CHRONOSORT_LOAD(rankS);
        CHRONOSORT_LOAD(rankA);
        CHRONOSORT_LOAD(rankB);
        CHRONOSORT_LOAD(handSizes);
        CHRONOSORT_LOAD(cpuLevels);
        CHRONOSORT_LOAD(comfortableGapYears);
        CHRONOSORT_LOAD(nearMissChance);
        CHRONOSORT_LOAD(cpus);
#undef CHRONOSORT_LOAD
    }

}    // namespace ChronoSort::Rules
