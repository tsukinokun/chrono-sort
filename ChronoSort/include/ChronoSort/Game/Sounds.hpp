//----------------------------------------------------------------------------
//! @file   Sounds.hpp
//! @brief  ゲーム全体で共有する SoundBank の取得
//! @detail シーンをまたいでも BGM を途切れさせないため、SoundBank は1つだけ作って使い回します。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Game/SoundBank.hpp>

namespace Tsukino::EngineIntegration {
    struct EngineContext;
}    // namespace Tsukino::EngineIntegration

namespace ChronoSort::Game {

    //! ゲーム全体で共有する SoundBank を返します。初回に作ります。
    //! @param  [in] context エンジンのコンテキスト。
    //! @return SoundBank。
    SoundBank& Sounds(Tsukino::EngineIntegration::EngineContext& context);

}    // namespace ChronoSort::Game
