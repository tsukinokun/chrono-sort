//----------------------------------------------------------------------------
//! @file   RevealSystem.hpp
//! @brief  出したカードの公開と判定の演出を進めるシステム
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

namespace ChronoSort::ECS {

    //! 出したカードの公開と判定の演出を、RevealComponent の段階に沿って進めます。一時停止中は進めません。
    class RevealSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace ChronoSort::ECS
