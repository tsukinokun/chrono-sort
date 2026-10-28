//----------------------------------------------------------------------------
//! @file   GameServices.hpp
//! @brief  シーンをまたいで共有するもの（音と設定）
//! @detail BGM をシーンの切り替えで途切れさせないため、音（SoundBank）と設定（Options）はシーンより長く生かします。
//!         WinMain がエンジンの初期化のあとに1つだけ作り、終わるときに（エンジンより先に）壊します。
//!         作ってから壊すまでの間は、どこからでも Services() で受け取れます。
//!         以前は関数の中の static で持っていたため、作られる時と壊される時（エンジンより後）が見えませんでした。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Game/Options.hpp>
#include <ChronoSort/Game/SoundBank.hpp>

namespace Tsukino::EngineIntegration {
    struct EngineContext;
}    // namespace Tsukino::EngineIntegration

namespace ChronoSort::Game {

    //! シーンをまたいで共有するもの（音と設定）です。
    class GameServices {
    public:

        //! 音を用意し、前回の設定を読んで音量へ反映します。作った時から Services() で受け取れます。
        //! @param  [in] context エンジンのコンテキスト（エンジンの初期化のあとに渡す）。
        explicit GameServices(Tsukino::EngineIntegration::EngineContext& context);

        //! Services() で受け取れないようにします。
        ~GameServices();

        GameServices(const GameServices&)            = delete;
        GameServices& operator=(const GameServices&) = delete;

        //! 効果音と BGM の再生窓口を返します。
        //! @return 再生窓口。
        SoundBank& Sounds() { return m_sounds; }

        //! 設定を返します（書き換えたら ApplyOptions で音量に反映し、StoreOptions で保存する）。
        //! @return 設定。
        Options& Settings() { return m_options; }

        //! 音量の設定を、エンジンと SoundBank に反映します。
        void ApplyOptions();

        //! 設定を Saves/Options.json に書き込みます。
        void StoreOptions() const;

    private:
        Tsukino::EngineIntegration::EngineContext& m_context;    // エンジン（全体の音量を渡す先）
        SoundBank                                  m_sounds;     // 効果音と BGM
        Options                                    m_options;    // 設定
    };

    //! 今あるシーンをまたいで共有するものを返します（WinMain が作ってから壊すまでの間だけ呼べる）。
    //! @return 共有するもの。
    GameServices& Services();

}    // namespace ChronoSort::Game
