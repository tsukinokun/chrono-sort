//----------------------------------------------------------------------------
//! @file   Options.cpp
//! @brief  設定の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Game/Options.hpp>

#include <ChronoSort/Game/Sounds.hpp>

#include <Tsukino/Audio/AudioManager.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <cereal/archives/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace ChronoSort::Game {

    namespace {
        constexpr const char* kOptionsDirectory = "Saves";
        constexpr const char* kOptionsPath      = "Saves/Options.json";

        //----------------------------------------------------------------------------
        //! スライダーの位置を音量に変えます。
        //! 耳は音量をほぼ対数で感じるので、位置をそのまま使うと線の右半分ではほとんど変わらない。
        //! 2乗にすると、線のどこを動かしても変化が感じられる。
        //! @param  [in] position スライダーの位置（0〜1）。
        //! @return 音量（0〜1）。
        //----------------------------------------------------------------------------
        float ToGain(float position) {
            const float clamped = std::clamp(position, 0.0f, 1.0f);
            return clamped * clamped;
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! 全シーンで共有する設定を返します。
    //----------------------------------------------------------------------------
    Options& CurrentOptions() {
        static Options options;
        return options;
    }

    //----------------------------------------------------------------------------
    //! 設定を読み込みます。
    //----------------------------------------------------------------------------
    void LoadOptions() {
        Options&      options = CurrentOptions();
        std::ifstream is(kOptionsPath);
        if(!is) {
            options = Options{};
            return;
        }
        try {
            cereal::JSONInputArchive archive(is);
            archive(cereal::make_nvp("options", options));
        } catch(const std::exception& e) {
            // 読めない設定で困るより、初期値で始めるほうがよい（次に閉じたとき書き直される）
            Tsukino::Core::Log::Warn(std::string("Failed to read options, using defaults: ") + e.what());
            options = Options{};
        }
    }

    //----------------------------------------------------------------------------
    //! 設定を書き込みます。
    //----------------------------------------------------------------------------
    void StoreOptions() {
        std::error_code error;
        std::filesystem::create_directories(kOptionsDirectory, error);
        std::ofstream os(kOptionsPath);
        if(!os) {
            Tsukino::Core::Log::Warn("Failed to write options.");
            return;
        }
        cereal::JSONOutputArchive archive(os);
        archive(cereal::make_nvp("options", CurrentOptions()));
    }

    //----------------------------------------------------------------------------
    //! 音量の設定を反映します。
    //----------------------------------------------------------------------------
    void ApplyOptions(Tsukino::EngineIntegration::EngineContext& context) {
        const Options& options = CurrentOptions();
        context.audioManager->SetMasterVolume(ToGain(options.master));
        SoundBank& sounds = Sounds(context);
        sounds.SetMusicVolume(ToGain(options.bgm));
        sounds.SetEffectsVolume(ToGain(options.sfx));    }

}    // namespace ChronoSort::Game
