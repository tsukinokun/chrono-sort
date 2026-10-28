//----------------------------------------------------------------------------
//! @file   MatchSession.cpp
//! @brief  レジストリに置いた対局の状態を受け取る関数の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Match/MatchSession.hpp>

namespace ChronoSort::Match {

    //----------------------------------------------------------------------------
    //! レジストリに置いた対局の状態を返します。
    //----------------------------------------------------------------------------
    MatchSession& Session(Tsukino::ECS::Registry& registry) {
        return *registry.GetContext<MatchSession*>();
    }

    //----------------------------------------------------------------------------
    //! レジストリが対局の状態を持っているかを返します。
    //----------------------------------------------------------------------------
    bool HasSession(Tsukino::ECS::Registry& registry) {
        return registry.HasContext<MatchSession*>();
    }

}    // namespace ChronoSort::Match
