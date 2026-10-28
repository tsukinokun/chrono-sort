//----------------------------------------------------------------------------
//! @file   MatchSession.cpp
//! @brief  レジストリに置いた対局の状態を受け取る関数の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Match/MatchSession.hpp>

#include <Tsukino/Core/ECS/Event/EventBus.hpp>

namespace ChronoSort::Match {

    //----------------------------------------------------------------------------
    //! レジストリに置いた対局の状態を返します。
    //----------------------------------------------------------------------------
    MatchSession& Session(Tsukino::ECS::Registry& registry) {
        return registry.GetContext<MatchSession>();
    }

    //----------------------------------------------------------------------------
    //! レジストリが対局の状態を持っているかを返します。
    //----------------------------------------------------------------------------
    bool HasSession(Tsukino::ECS::Registry& registry) {
        return registry.HasContext<MatchSession>();
    }

    //----------------------------------------------------------------------------
    //! 対局の出来事を知らせる先を返します。
    //----------------------------------------------------------------------------
    Tsukino::ECS::EventBus& Events(Tsukino::ECS::Registry& registry) {
        // Scene が自分の EventBus をコンテキストに置いている
        return *registry.GetContext<Tsukino::ECS::EventBus*>();
    }

}    // namespace ChronoSort::Match
