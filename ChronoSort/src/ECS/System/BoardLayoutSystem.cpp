//----------------------------------------------------------------------------
//! @file   BoardLayoutSystem.cpp
//! @brief  卓の上のカードを置き場所どおりに並べるシステムの実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/System/BoardLayoutSystem.hpp>

#include <ChronoSort/Board/BoardLayout.hpp>

namespace ChronoSort::ECS {

    //----------------------------------------------------------------------------
    //! 目標の姿勢を書き込みます。
    //----------------------------------------------------------------------------
    void BoardLayoutSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        // 卓の無いシーン（タイトル・クレジット）では並べる物が無い
        if(!registry.HasContext<Board::BoardState>()) {
            return;
        }
        Board::UpdateBoardLayout(registry, deltaTime);
    }

}    // namespace ChronoSort::ECS
