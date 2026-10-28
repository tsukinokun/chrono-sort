//----------------------------------------------------------------------------
//! @file   CpuTurnSystem.cpp
//! @brief  CPU の手番を進めるシステムの実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/System/CpuTurnSystem.hpp>

#include <ChronoSort/Board/BoardView.hpp>
#include <ChronoSort/Game/TuningSet.hpp>
#include <ChronoSort/Match/MatchActions.hpp>
#include <ChronoSort/Match/MatchEvents.hpp>

#include <Tsukino/Core/ECS/Event/EventBus.hpp>

#include <algorithm>
#include <random>

namespace ChronoSort::ECS {

    //----------------------------------------------------------------------------
    //! CPU の手番を進めます。
    //----------------------------------------------------------------------------
    void CpuTurnSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        if(!Match::HasSession(registry)) {
            return;
        }
        Match::MatchSession& session = Match::Session(registry);
        Match::CpuThinking&  cpu     = session.cpu;
        if(session.paused || session.phase != Match::MatchPhase::CpuTurn) {
            return;
        }

        if(!cpu.active) {
            // 手に取って考え始める。自信が無いほど長く迷い、途中で置き先の候補を行き来する
            cpu        = Match::CpuThinking{};
            cpu.active = true;
            cpu.player = session.match.CurrentPlayerIndex();
            cpu.move   = Rules::ChooseCpuMove(session.match.Players()[cpu.player], session.match.GetTimeline(), session.rng, session.config.rules);

            const Match::CpuTuning&               tuning = Game::Tuning().Cpu();
            const float                           doubt  = 1.0f - cpu.move.confidence;
            std::uniform_real_distribution<float> unit(0.0f, 1.0f);
            cpu.think = tuning.thinkBase + doubt * tuning.thinkPerDoubt + unit(session.rng) * tuning.thinkRandom;

            const size_t slotCount = session.match.GetTimeline().SlotCount();
            const int    hops      = std::max(1, tuning.hopsBase + static_cast<int>(doubt * tuning.hopsPerDoubt));
            for(int i = 0; i < hops; ++i) {
                const float  at        = cpu.think * (tuning.hopStart + tuning.hopSpan * static_cast<float>(i) / static_cast<float>(hops));
                const int    wander    = std::uniform_int_distribution<int>(-tuning.wander, tuning.wander)(session.rng);
                const size_t candidate = static_cast<size_t>(std::clamp(static_cast<int>(cpu.move.slot) + wander, 0, static_cast<int>(slotCount) - 1));
                cpu.hops.emplace_back(at, candidate);
            }
            session.board->Hand(cpu.player).lifted = static_cast<int>(cpu.move.handIndex);
            session.events->Publish(Match::CpuThinkingStartedEvent{cpu.player});
            return;
        }

        cpu.time += deltaTime;
        while(cpu.nextHop < cpu.hops.size() && cpu.hops[cpu.nextHop].first <= cpu.time) {
            const size_t slot = cpu.hops[cpu.nextHop].second;
            session.board->SetGap(slot);
            session.board->SetMarker(true);
            session.events->Publish(Match::CpuHoveredSlotEvent{slot});
            ++cpu.nextHop;
        }
        if(cpu.time >= cpu.think) {
            cpu.active                             = false;
            session.board->Hand(cpu.player).lifted = -1;
            Match::CommitPlay(registry, cpu.player, cpu.move.handIndex, cpu.move.slot);
        }
    }

}    // namespace ChronoSort::ECS
