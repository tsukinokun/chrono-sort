//----------------------------------------------------------------------------
//! @file   Options.hpp
//! @brief  設定（音量・演出の速さ・画面の揺れ）
//! @detail Saves/Options.json に保存します。記録（SaveData）とは別のファイルなので、
//!         設定の項目を増やしても記録の読み込みに影響しません。
//!         値は起動時に GameServices が1回だけ読み、Services().Settings() で全シーンが共有します。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/IO/CerealOptional.hpp>

#include <cereal/cereal.hpp>

namespace ChronoSort::Game {

    //! 画質の選び方です（値は Options.json に保存され、設定の札の値にもなる）。
    enum class GraphicsQualityChoice : int {
        Auto   = 0,    // 動く環境に合わせて自動で決める
        High   = 1,    // 高
        Medium = 2,    // 中
        Low    = 3,    // 低
    };

    //! 設定です。
    struct Options {
        float master      = 1.0f;     // 全体の音量（0〜1。スライダーの位置）
        float bgm         = 0.8f;     // BGM の音量（0〜1）
        float sfx         = 1.0f;     // 効果音の音量（0〜1）
        bool  fastEffects = false;    // 演出を速くするか（自分の手番以外の時間を早回しする）
        bool  cameraShake = true;     // 判定などでカメラを揺らすか
        int   graphicsQuality = static_cast<int>(GraphicsQualityChoice::Auto);    // 画質（GraphicsQualityChoice の値）

        template <class Archive>
        void save(Archive& archive) const {
            archive(cereal::make_nvp("master", master),
                    cereal::make_nvp("bgm", bgm),
                    cereal::make_nvp("sfx", sfx),
                    cereal::make_nvp("fastEffects", fastEffects),
                    cereal::make_nvp("cameraShake", cameraShake),
                    cereal::make_nvp("graphicsQuality", graphicsQuality));
        }

        template <class Archive>
        void load(Archive& archive) {
            // あとから足した項目が無い古いファイルでも、残りの設定を捨てずに読む
            archive(cereal::make_nvp("master", master),
                    cereal::make_nvp("bgm", bgm),
                    cereal::make_nvp("sfx", sfx),
                    cereal::make_nvp("fastEffects", fastEffects),
                    cereal::make_nvp("cameraShake", cameraShake));
            Tsukino::IO::LoadOptional(archive, "graphicsQuality", graphicsQuality);
        }
    };

    //! Saves/Options.json から設定を読み込みます。無ければ・読めなければ初期値を返します。
    //! @return 設定。
    Options LoadOptions();

    //! 設定を Saves/Options.json に書き込みます。
    //! @param  [in] options 設定。
    void StoreOptions(const Options& options);

    //! スライダーの位置を音量に変えます（2乗。耳の感じ方に近づける）。
    //! @param  [in] position スライダーの位置（0〜1）。
    //! @return 音量（0〜1）。
    float VolumeGain(float position);

}    // namespace ChronoSort::Game
