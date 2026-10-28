//----------------------------------------------------------------------------
//! @file   GameServices.cpp
//! @brief  シーンをまたいで共有するものの実装
//----------------------------------------------------------------------------
#include <ChronoSort/Game/GameServices.hpp>

#include <Tsukino/Audio/AudioManager.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <cassert>

namespace ChronoSort::Game {

    namespace {
        GameServices* s_current = nullptr;    // 今ある共有するもの（WinMain が作ってから壊すまで）
    }    // namespace

    //----------------------------------------------------------------------------
    //! 音を用意し、前回の設定を読んで音量へ反映します。
    //----------------------------------------------------------------------------
    GameServices::GameServices(Tsukino::EngineIntegration::EngineContext& context)
        : m_context(context)
        , m_sounds(*context.assetManager, *context.audioManager)
        , m_options(LoadOptions()) {
        assert(s_current == nullptr && "GameServices: only one instance may exist");
        s_current = this;
        // 最初の音が鳴る前に、前回の音量にしておく
        ApplyOptions();
    }

    //----------------------------------------------------------------------------
    //! Services() で受け取れないようにします。
    //----------------------------------------------------------------------------
    GameServices::~GameServices() {
        s_current = nullptr;
    }

    //----------------------------------------------------------------------------
    //! 音量の設定を、エンジンと SoundBank に反映します。
    //----------------------------------------------------------------------------
    void GameServices::ApplyOptions() {
        m_context.audioManager->SetMasterVolume(VolumeGain(m_options.master));
        m_sounds.SetMusicVolume(VolumeGain(m_options.bgm));
        m_sounds.SetEffectsVolume(VolumeGain(m_options.sfx));
    }

    //----------------------------------------------------------------------------
    //! 設定を書き込みます。
    //----------------------------------------------------------------------------
    void GameServices::StoreOptions() const {
        Game::StoreOptions(m_options);
    }

    //----------------------------------------------------------------------------
    //! 今あるシーンをまたいで共有するものを返します。
    //----------------------------------------------------------------------------
    GameServices& Services() {
        assert(s_current != nullptr && "GameServices: not created yet (WinMain creates it after the engine)");
        return *s_current;
    }

}    // namespace ChronoSort::Game
