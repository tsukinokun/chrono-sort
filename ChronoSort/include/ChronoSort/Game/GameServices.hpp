//----------------------------------------------------------------------------
//! @file   GameServices.hpp
//! @brief  シーンをまたいで共有するもの（調整値・音・設定・記録）
//! @detail BGM をシーンの切り替えで途切れさせないため、音（SoundBank）と設定（Options）はシーンより長く生かします。
//!         WinMain がエンジンの初期化のあとに1つだけ作り、終わるときに（エンジンより先に）壊します。
//!         作ってから壊すまでの間は、どこからでも Services() で受け取れます。
//!         以前は関数の中の static で持っていたため、作られる時と壊される時（エンジンより後）が見えませんでした。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Game/Options.hpp>
#include <ChronoSort/Game/SaveData.hpp>
#include <ChronoSort/Game/SoundBank.hpp>
#include <ChronoSort/Game/TuningSet.hpp>

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

        //! 調整値を返します（Game::Tuning() と同じ）。
        //! @return 調整値。
        const TuningSet& Tunings() const { return m_tunings; }

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

        //! 記録を返します（起動時に1回読む。書き換えたら StoreSave で保存する）。
        //! @return 記録。
        //! @note   タイトルの範囲の選び直しと対局の終わりの両方が、この1つを書き換えて保存する
        //!         （それぞれが読んで書き戻すと、後から書いた方が先の変更を消してしまうため）
        SaveData& Save() { return m_save; }

        //! 記録を Saves/ChronoSort.json に書き込みます。
        void StoreSave() const;

    private:
        Tsukino::EngineIntegration::EngineContext& m_context;    // エンジン（全体の音量を渡す先）
        TuningSet                                  m_tunings;    // 調整値（音の合図も含むので、音より先に作る）
        SoundBank                                  m_sounds;     // 効果音と BGM
        Options                                    m_options;    // 設定
        SaveData                                   m_save;       // 記録
    };

    //! 今あるシーンをまたいで共有するものを返します（WinMain が作ってから壊すまでの間だけ呼べる）。
    //! @return 共有するもの。
    GameServices& Services();

}    // namespace ChronoSort::Game
