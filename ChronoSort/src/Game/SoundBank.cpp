//----------------------------------------------------------------------------
//! @file   SoundBank.cpp
//! @brief  効果音と BGM の再生窓口の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Game/SoundBank.hpp>

#include <Tsukino/Audio/AudioManager.hpp>
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Engine/Asset/Audio/AudioAsset.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>

#include <filesystem>
#include <memory>

namespace ChronoSort::Game {

    //----------------------------------------------------------------------------
    //! アセット管理と再生先を受け取ります。
    //----------------------------------------------------------------------------
    SoundBank::SoundBank(Tsukino::Asset::AssetManager& assetManager, Tsukino::Audio::AudioManager& audioManager)
        : m_assetManager(assetManager)
        , m_audioManager(audioManager) {
    }

    //----------------------------------------------------------------------------
    //! 効果音を鳴らします。
    //----------------------------------------------------------------------------
    void SoundBank::Play(const std::string& name, float volume) {
        const Tsukino::Asset::AssetHandle handle = Load("ChronoSort/Assets/Audio/Sfx/" + name + ".wav");
        if(!handle.IsValid()) {
            return;
        }
        if(auto asset = std::dynamic_pointer_cast<Tsukino::Asset::AudioAsset>(m_assetManager.Get(handle))) {
            m_audioManager.Play(*asset, false, volume * m_effectsVolume);
        }
    }

    //----------------------------------------------------------------------------
    //! BGM をループ再生します。
    //----------------------------------------------------------------------------
    void SoundBank::PlayBgm(const std::string& name, float volume) {
        const Tsukino::Asset::AssetHandle handle = Load("ChronoSort/Assets/Audio/Bgm/" + name + ".mp3");
        if(handle == m_bgm) {
            return;
        }
        StopBgm();
        if(!handle.IsValid()) {
            return;
        }
        if(auto asset = std::dynamic_pointer_cast<Tsukino::Asset::AudioAsset>(m_assetManager.Get(handle))) {
            m_audioManager.Play(*asset, true, volume * m_musicVolume);
            m_bgm           = handle;
            m_bgmBaseVolume = volume;
        }
    }

    //----------------------------------------------------------------------------
    //! BGM の音量の倍率を設定します。
    //----------------------------------------------------------------------------
    void SoundBank::SetMusicVolume(float volume) {
        m_musicVolume = volume;
        if(!m_bgm.IsValid()) {
            return;
        }
        if(auto asset = std::dynamic_pointer_cast<Tsukino::Asset::AudioAsset>(m_assetManager.Get(m_bgm))) {
            m_audioManager.SetVolume(*asset, m_bgmBaseVolume * m_musicVolume);
        }
    }

    //----------------------------------------------------------------------------
    //! 効果音の音量の倍率を設定します。
    //----------------------------------------------------------------------------
    void SoundBank::SetEffectsVolume(float volume) {
        m_effectsVolume = volume;
    }

    //----------------------------------------------------------------------------
    //! BGM を止めます。
    //----------------------------------------------------------------------------
    void SoundBank::StopBgm() {
        if(!m_bgm.IsValid()) {
            return;
        }
        if(auto asset = std::dynamic_pointer_cast<Tsukino::Asset::AudioAsset>(m_assetManager.Get(m_bgm))) {
            m_audioManager.Stop(*asset);
        }
        m_bgm = Tsukino::Asset::AssetHandle::Invalid();
    }

    //----------------------------------------------------------------------------
    //! 音を読み込みます。
    //----------------------------------------------------------------------------
    Tsukino::Asset::AssetHandle SoundBank::Load(const std::string& path) {
        if(const auto it = m_cache.find(path); it != m_cache.end()) {
            return it->second;
        }
        // 素材が揃う前でも動くように、無いファイルは読み込まずに無効のまま覚えておく
        Tsukino::Asset::AssetHandle handle = Tsukino::Asset::AssetHandle::Invalid();
        if(std::filesystem::exists(Tsukino::Core::Path(path).string())) {
            handle = m_assetManager.Load(Tsukino::Core::Path(path));
        }
        m_cache.emplace(path, handle);
        return handle;
    }

}    // namespace ChronoSort::Game
