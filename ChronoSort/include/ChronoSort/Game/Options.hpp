//----------------------------------------------------------------------------
//! @file   Options.hpp
//! @brief  設定（音量・演出の速さ・画面の揺れ）
//! @detail Saves/Options.json に保存します。記録（SaveData）とは別のファイルなので、
//!         設定の項目を増やしても記録の読み込みに影響しません。
//!         値は起動時に GameServices が1回だけ読み、Services().Settings() で全シーンが共有します。
//----------------------------------------------------------------------------
#pragma once
#include <cereal/cereal.hpp>

namespace ChronoSort::Game {

    //! 設定です。
    struct Options {
        float master      = 1.0f;     // 全体の音量（0〜1。スライダーの位置）
        float bgm         = 0.8f;     // BGM の音量（0〜1）
        float sfx         = 1.0f;     // 効果音の音量（0〜1）
        bool  fastEffects = false;    // 演出を速くするか（自分の手番以外の時間を早回しする）
        bool  cameraShake = true;     // 判定などでカメラを揺らすか

        template <class Archive>
        void serialize(Archive& archive) {
            archive(cereal::make_nvp("master", master),
                    cereal::make_nvp("bgm", bgm),
                    cereal::make_nvp("sfx", sfx),
                    cereal::make_nvp("fastEffects", fastEffects),
                    cereal::make_nvp("cameraShake", cameraShake));
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
