//----------------------------------------------------------------------------
//! @file   CpuTurnSystem.hpp
//! @brief  CPU の手番を進めるシステム
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

namespace ChronoSort::ECS {

    //! CPU の手番：手札を手に取って迷い、置き先を決めて出します。一時停止中は進めません。
    class CpuTurnSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace ChronoSort::ECS
