//----------------------------------------------------------------------------
//! @file   Options.cpp
//! @brief  設定の読み書きの実装
//----------------------------------------------------------------------------
#include <ChronoSort/Game/Options.hpp>

#include <Tsukino/Core/Log.hpp>

#include <cereal/archives/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace ChronoSort::Game {

    namespace {
        constexpr const char* kOptionsDirectory = "Saves";
        constexpr const char* kOptionsPath      = "Saves/Options.json";
    }    // namespace

    //----------------------------------------------------------------------------
    //! 設定を読み込みます。
    //----------------------------------------------------------------------------
    Options LoadOptions() {
        Options       options;
        std::ifstream is(kOptionsPath);
        if(!is) {
            return options;
        }
        try {
            cereal::JSONInputArchive archive(is);
            archive(cereal::make_nvp("options", options));
        } catch(const std::exception& e) {
            // 読めない設定で困るより、初期値で始めるほうがよい（次に閉じたとき書き直される）
            Tsukino::Core::Log::Warn(std::string("Failed to read options, using defaults: ") + e.what());
            options = Options{};
        }
        return options;
    }

    //----------------------------------------------------------------------------
    //! 設定を書き込みます。
    //----------------------------------------------------------------------------
    void StoreOptions(const Options& options) {
        std::error_code error;
        std::filesystem::create_directories(kOptionsDirectory, error);
        std::ofstream os(kOptionsPath);
        if(!os) {
            Tsukino::Core::Log::Warn("Failed to write options.");
            return;
        }
        cereal::JSONOutputArchive archive(os);
        archive(cereal::make_nvp("options", options));
    }

    //----------------------------------------------------------------------------
    //! スライダーの位置を音量に変えます。
    //----------------------------------------------------------------------------
    float VolumeGain(float position) {
        // 耳は音量をほぼ対数で感じるので、位置をそのまま使うと線の右半分ではほとんど変わらない。
        // 2乗にすると、線のどこを動かしても変化が感じられる
        const float clamped = std::clamp(position, 0.0f, 1.0f);
        return clamped * clamped;
    }

}    // namespace ChronoSort::Game
