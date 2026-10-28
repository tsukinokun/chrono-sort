//----------------------------------------------------------------------------
//! @file   MatchFeedbackSystem.hpp
//! @brief  対局の出来事に音・粒子・画面 UI で返すシステム
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Event/ScopedConnection.hpp>
#include <Tsukino/Core/ECS/System/ISystem.hpp>

#include <vector>

namespace Tsukino::ECS {
    class EventBus;
}    // namespace Tsukino::ECS

namespace ChronoSort::ECS {

    //! 対局の出来事を受けて、音・粒子・画面 UI・発光・カメラの揺れで返します。
    //! 自分では何も進めません（Update は空）。購読は自分の寿命の間だけ続きます。
    class MatchFeedbackSystem : public Tsukino::ECS::ISystem {
    public:

        //! 出来事の購読を始めます。
        //! @param  [in] registry 対局のシーンのレジストリ（MatchSession を持っていること）。
        //! @param  [in] events   対局のシーンの EventBus。
        MatchFeedbackSystem(Tsukino::ECS::Registry& registry, Tsukino::ECS::EventBus& events);

        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        std::vector<Tsukino::ECS::ScopedConnection> m_connections;    // 購読（破棄で解除される）
    };

}    // namespace ChronoSort::ECS
