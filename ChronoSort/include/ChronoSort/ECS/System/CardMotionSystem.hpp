//----------------------------------------------------------------------------
//! @file   CardMotionSystem.hpp
//! @brief  カードを目標姿勢へ滑らかに近づけるシステム
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

namespace ChronoSort::ECS {

    //! CardMotionComponent の目標へ、Transform を指数補間で近づけます。
    //! フレームレートに依存しないよう、補間率は 1 - exp(-速さ × 経過時間) で求めます。
    class CardMotionSystem : public Tsukino::ECS::ISystem {
    public:

        //! カードの Transform を目標へ近づけます。
        //! @param  [in,out] registry  レジストリ。
        //! @param  [in]     deltaTime 経過時間（秒）。
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace ChronoSort::ECS
