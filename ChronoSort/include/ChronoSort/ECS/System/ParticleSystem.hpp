//----------------------------------------------------------------------------
//! @file   ParticleSystem.hpp
//! @brief  粒子を動かし、寿命が来たら破棄するシステム
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

namespace ChronoSort::ECS {

    //! 粒子を動かし、大きさと色を寿命に合わせて変え、寿命が来たら破棄します。
    class ParticleSystem : public Tsukino::ECS::ISystem {
    public:

        //! 粒子を更新します。
        //! @param  [in,out] registry  レジストリ。
        //! @param  [in]     deltaTime 経過時間（秒）。
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace ChronoSort::ECS
