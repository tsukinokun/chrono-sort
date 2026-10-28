//----------------------------------------------------------------------------
//! @file   SoundBank.hpp
//! @brief  効果音と BGM の再生窓口
//! @detail 合図の名前（"play.flip" など）で音を鳴らします。どのファイルをどの音量で鳴らすかは
//!         Sounds.json（SoundTuning）で決めます。ファイルが無い音は黙って無視するので、
//!         音素材が揃う前でもゲームは動きます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Audio/AudioManager.hpp>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>

#include <optional>
#include <string>
#include <unordered_map>

namespace Tsukino::Asset {
    class AssetManager;
}    // namespace Tsukino::Asset

namespace ChronoSort::Game {

    //! 効果音と BGM の再生窓口です。
    //! @note   BGM と効果音はエンジンの別のバスで鳴らし、設定の音量はバスの音量として掛ける。
    class SoundBank {
    public:

        //! BGM を鳴らすバス
        static constexpr Tsukino::Audio::AudioBus kMusicBus = 1;

        //! 効果音を鳴らすバス
        static constexpr Tsukino::Audio::AudioBus kEffectsBus = 2;

        //! アセット管理と再生先を受け取ります。
        //! @param  [in] assetManager アセット管理。
        //! @param  [in] audioManager 再生先。
        SoundBank(Tsukino::Asset::AssetManager& assetManager, Tsukino::Audio::AudioManager& audioManager);

        //! 効果音を、合図の名前で鳴らします（Sounds.json の cues。無い名前なら警告を出して鳴らさない）。
        //! @param  [in] cue 合図の名前。
        void Play(const std::string& cue);

        //! BGM を、合図の名前でループ再生します（Sounds.json の music）。既に流れている BGM は止めます。
        //! @param  [in] cue 合図の名前。
        void PlayBgm(const std::string& cue);

        //! 効果音の合図を、読み込んだ音と音量にします（エンジンの UIButtonSoundComponent などに渡すとき用）。
        //! @param  [in]  cue    合図の名前。
        //! @param  [out] volume 合図の音量。
        //! @return 音（合図が無い・ファイルが無いなら std::nullopt）。
        std::optional<Tsukino::Asset::AssetHandle> ResolveCue(const std::string& cue, float& volume);

        //! BGM を止めます。
        void StopBgm();

        //! BGM の音量の倍率を設定します（BGM のバスの音量）。流れている BGM にもすぐ反映します。
        //! @param  [in] volume 倍率（0〜1）。
        void SetMusicVolume(float volume);

        //! 効果音の音量の倍率を設定します（効果音のバスの音量）。
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
    };

}    // namespace ChronoSort::Game
