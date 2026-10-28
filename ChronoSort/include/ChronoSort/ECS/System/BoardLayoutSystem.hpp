//----------------------------------------------------------------------------
//! @file   BoardLayoutSystem.hpp
//! @brief  卓の上のカードを置き場所どおりに並べるシステム
//! @detail CardPlacementComponent（カードごとの置き場所）と BoardState（卓全体の状態）を読み、
//!         場の列・手札・捨て札の目標の姿勢を CardMotion に書きます。実際の移動は CardMotionSystem が行うので、
//!         同じ順番の中でそれより前に並べます。BoardState が無いシーンでは何もしません。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

namespace ChronoSort::ECS {

    //! 卓の上のカードを置き場所どおりに並べるシステムです。
    class BoardLayoutSystem : public Tsukino::ECS::ISystem {
    public:

        //! 目標の姿勢を書き込みます。
        //! @param  [in,out] registry  レジストリ。
        //! @param  [in]     deltaTime 経過時間（秒）。
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace ChronoSort::ECS
