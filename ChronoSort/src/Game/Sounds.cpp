//----------------------------------------------------------------------------
//! @file   Sounds.cpp
//! @brief  ゲーム全体で共有する SoundBank の取得の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Game/Sounds.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

namespace ChronoSort::Game {

    //----------------------------------------------------------------------------
    //! ゲーム全体で共有する SoundBank を返します。
    //----------------------------------------------------------------------------
    SoundBank& Sounds(Tsukino::EngineIntegration::EngineContext& context) {
        // AssetManager と AudioManager はエンジンの寿命の間ずっと生きているので、関数内の static で持つ
        static SoundBank bank(*context.assetManager, *context.audioManager);
        return bank;
    }

}    // namespace ChronoSort::Game
