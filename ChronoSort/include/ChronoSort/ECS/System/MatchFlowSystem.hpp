//----------------------------------------------------------------------------
//! @file   MatchFlowSystem.hpp
//! @brief  時間を置いた段取りを進めるシステム
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

namespace ChronoSort::ECS {

    //! 時間を置いた段取り（手番を始める・回す・結果を出す）と、見た目だけの時間差を進めます。
    //! 一時停止中は進めません。
    class MatchFlowSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace ChronoSort::ECS
