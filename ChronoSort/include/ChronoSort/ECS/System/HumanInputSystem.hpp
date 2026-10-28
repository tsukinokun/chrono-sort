//----------------------------------------------------------------------------
//! @file   HumanInputSystem.hpp
//! @brief  人間の入力を処理するシステム
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

namespace ChronoSort::ECS {

    //! 人間の入力：手札のホバー・ドラッグ・クリックで選ぶ・置き先の予告、列のホイール送り、放置時間。
    class HumanInputSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace ChronoSort::ECS
