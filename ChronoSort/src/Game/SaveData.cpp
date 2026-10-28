//----------------------------------------------------------------------------
//! @file   SaveData.cpp
//! @brief  記録の保存の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Game/SaveData.hpp>

#include <Tsukino/Core/Log.hpp>

#include <cereal/archives/json.hpp>

#include <filesystem>
#include <fstream>

namespace ChronoSort::Game {

    namespace {
        constexpr const char* kSaveDirectory = "Saves";
        constexpr const char* kSavePath      = "Saves/ChronoSort.json";

        //! 範囲を分ける前（version 1）の記録の形です。読み込みにだけ使います。
        struct LegacySaveData {
            int bestScore  = 0;
            int bestPlaced = 0;
            int bestStreak = 0;
            int versusWins = 0;

            template <class Archive>
            void serialize(Archive& archive) {
                archive(cereal::make_nvp("bestScore", bestScore),
                        cereal::make_nvp("bestPlaced", bestPlaced),
                        cereal::make_nvp("bestStreak", bestStreak),
                        cereal::make_nvp("versusWins", versusWins));
            }
        };

        //----------------------------------------------------------------------------
        //! 記録のファイルを指定した形で読みます。
        //! @param  [out] out 読んだ記録。
        //! @return 読めたら true。
        //----------------------------------------------------------------------------
        template <class T>
        bool TryRead(T& out) {
            std::ifstream is(kSavePath);
            if(!is) {
                return false;
            }
            try {
                cereal::JSONInputArchive archive(is);
                archive(cereal::make_nvp("save", out));
                return true;
            } catch(const std::exception&) {
                return false;
            }
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! 記録を読み込みます。
    //----------------------------------------------------------------------------
    SaveData LoadSaveData() {
        SaveData data;
        if(!std::filesystem::exists(kSavePath)) {
            return data;
        }
        if(TryRead(data)) {
            return data;
        }

        // 範囲を分ける前の記録なら、通史の記録として引き継ぐ（次の保存で新しい形になる）
        LegacySaveData legacy;
        if(TryRead(legacy)) {
            data                = SaveData{};
            data.all.bestScore  = legacy.bestScore;
            data.all.bestPlaced = legacy.bestPlaced;
            data.all.bestStreak = legacy.bestStreak;
            data.versusWins     = legacy.versusWins;
            Tsukino::Core::Log::Info("Migrated save data from version 1.");
            return data;
        }

        // 壊れた記録で起動できなくなるより、記録を諦めて初期値で続けるほうがよい
        Tsukino::Core::Log::Warn("Failed to read save data, starting fresh.");
        return SaveData{};
    }

    //----------------------------------------------------------------------------
    //! 記録を書き込みます。
    //----------------------------------------------------------------------------
    void StoreSaveData(const SaveData& data) {
        std::error_code error;
        std::filesystem::create_directories(kSaveDirectory, error);
        std::ofstream os(kSavePath);
        if(!os) {
            Tsukino::Core::Log::Warn("Failed to write save data.");
            return;
        }
        cereal::JSONOutputArchive archive(os);
        archive(cereal::make_nvp("save", data));
    }

}    // namespace ChronoSort::Game
