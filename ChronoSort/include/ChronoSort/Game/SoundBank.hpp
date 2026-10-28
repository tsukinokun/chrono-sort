//----------------------------------------------------------------------------
//! @file   SoundBank.hpp
//! @brief  効果音と BGM の再生窓口
//! @detail 名前で音を鳴らします。ファイルが無い音は黙って無視するので、
//!         音素材が揃う前でもゲームは動きます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>

#include <string>
#include <unordered_map>

namespace Tsukino::Asset {
    class AssetManager;
}    // namespace Tsukino::Asset

namespace Tsukino::Audio {
    class AudioManager;
}    // namespace Tsukino::Audio

namespace ChronoSort::Game {

    //! 効果音と BGM の再生窓口です。
    class SoundBank {
    public:

        //! アセット管理と再生先を受け取ります。
        //! @param  [in] assetManager アセット管理。
        //! @param  [in] audioManager 再生先。
        SoundBank(Tsukino::Asset::AssetManager& assetManager, Tsukino::Audio::AudioManager& audioManager);

        //! 効果音を鳴らします（ChronoSort/Assets/Audio/Sfx/<name>.wav）。
        //! @param  [in] name   音の名前。
        //! @param  [in] volume 音量（0〜1）。
        void Play(const std::string& name, float volume = 1.0f);

        //! BGM をループ再生します（ChronoSort/Assets/Audio/Bgm/<name>.mp3）。既に流れている BGM は止めます。
        //! @param  [in] name   曲の名前。
        //! @param  [in] volume 音量（0〜1）。
        void PlayBgm(const std::string& name, float volume = 0.45f);

        //! BGM を止めます。
        void StopBgm();

        //! BGM の音量の倍率を設定します。流れている BGM にもすぐ反映します。
        //! @param  [in] volume 倍率（0〜1）。
        void SetMusicVolume(float volume);

        //! 効果音の音量の倍率を設定します（次に鳴らす音から効く）。
        //! @param  [in] volume 倍率（0〜1）。
        void SetEffectsVolume(float volume);

    private:

        //! 音を読み込みます（読み込み済みならそれを返します）。
        //! @param  [in] path ワークスペースルートからの相対パス。
        //! @return ハンドル。ファイルが無ければ無効なハンドル。
        Tsukino::Asset::AssetHandle Load(const std::string& path);

        Tsukino::Asset::AssetManager&                                m_assetManager;    // アセット管理
        Tsukino::Audio::AudioManager&                                m_audioManager;    // 再生先
        std::unordered_map<std::string, Tsukino::Asset::AssetHandle> m_cache;           // 読み込み済みの音
        Tsukino::Asset::AssetHandle                                  m_bgm;             // 再生中の BGM
        float                                                        m_bgmBaseVolume = 1.0f;    // 再生中の BGM の曲ごとの音量
        float                                                        m_musicVolume   = 1.0f;    // BGM の音量の倍率（設定）
        float                                                        m_effectsVolume = 1.0f;    // 効果音の音量の倍率（設定）
    };

}    // namespace ChronoSort::Game
