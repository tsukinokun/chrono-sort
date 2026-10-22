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
    }    // namespace

    //----------------------------------------------------------------------------
    //! 記録を読み込みます。
    //----------------------------------------------------------------------------
    SaveData LoadSaveData() {
        SaveData      data;
        std::ifstream is(kSavePath);
        if(!is) {
            return data;
        }
        try {
            cereal::JSONInputArchive archive(is);
            archive(cereal::make_nvp("save", data));
        } catch(const std::exception& e) {
            // 壊れた記録で起動できなくなるより、記録を諦めて初期値で続けるほうがよい
            Tsukino::Core::Log::Warn(std::string("Failed to read save data, starting fresh: ") + e.what());
            data = SaveData{};
        }
        return data;
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
