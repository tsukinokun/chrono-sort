//----------------------------------------------------------------------------
//! @file   MatchFlowSystem.cpp
//! @brief  時間を置いた段取りを進めるシステムの実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/System/MatchFlowSystem.hpp>

#include <ChronoSort/Match/MatchActions.hpp>
#include <ChronoSort/Match/MatchEvents.hpp>

#include <Tsukino/Core/ECS/Event/EventBus.hpp>

namespace ChronoSort::ECS {

    //----------------------------------------------------------------------------
    //! 時間を置いた段取りと、見た目だけの時間差を進めます。
    //----------------------------------------------------------------------------
    void MatchFlowSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        if(!Match::HasSession(registry)) {
            return;
        }
        Match::MatchSession& session = Match::Session(registry);
        if(session.paused) {
            return;
        }
        session.sequencer.Update(deltaTime);

        if(session.step == Match::MatchStep::None) {
            return;
        }
        session.stepTimer -= deltaTime;
        if(session.stepTimer > 0.0f) {
            return;
        }
        // 段取りの中から次の段取りを予約できるよう、先に空けてから行う
        const Match::MatchStep step = session.step;
        session.step               = Match::MatchStep::None;
        switch(step) {
        case Match::MatchStep::StartTurn:
            Match::StartTurn(registry);
            break;
        case Match::MatchStep::AdvanceTurn:
            Match::AdvanceTurn(registry);
            break;
        case Match::MatchStep::ShowResult:
            session.events->Publish(Match::ResultRequestedEvent{});
            break;
        default:
            break;
        }
    }

}    // namespace ChronoSort::ECS
