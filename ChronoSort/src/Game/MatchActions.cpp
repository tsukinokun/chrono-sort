//----------------------------------------------------------------------------
//! @file   MatchActions.cpp
//! @brief  対局を進める操作の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Game/MatchActions.hpp>

#include <ChronoSort/Board/BoardView.hpp>
#include <ChronoSort/ECS/CardMotion.hpp>
#include <ChronoSort/ECS/Component/RevealComponent.hpp>
#include <ChronoSort/ECS/System/CardGlowSystem.hpp>
#include <ChronoSort/Game/FrameMonitor.hpp>
#include <ChronoSort/Game/MatchEvents.hpp>
#include <ChronoSort/Game/SaveData.hpp>

#include <Tsukino/Core/ECS/Event/EventBus.hpp>

#include <algorithm>

namespace ChronoSort::Game {

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

    //----------------------------------------------------------------------------
    //! 場の1枚目と手札を、山札から順に配ります。
    //----------------------------------------------------------------------------
    void DealOpening(Tsukino::ECS::Registry& registry) {
        MatchSession& session = Session(registry);
        session.phase         = MatchPhase::Dealing;

        // 場の1枚目：山札から飛んで、空中で裏返って年号を見せる
        const Rules::CardData&     first = session.match.GetTimeline().Cards().front();
        const Tsukino::ECS::Entity card  = session.board->SpawnAtDeck(first);
        session.board->InsertIntoRow(0, card);
        session.board->ApplyLayoutNow();
        ECS::CardMotion::Fly(registry, card, 0.7f, 120.0f, 0.2f);
        session.events->Publish(OpeningCardDealtEvent{card});

        const float lastArrival = DealToHands(registry, 1.1f);
        session.Schedule(MatchStep::StartTurn, lastArrival + 0.3f);
    }

    //----------------------------------------------------------------------------
    //! 手札の見た目を Rules::Match の手札に合わせます。
    //----------------------------------------------------------------------------
    float DealToHands(Tsukino::ECS::Registry& registry, float delay) {
        MatchSession& session = Session(registry);
        float         last    = delay;
        float         stagger = delay;

        // 1人ずつではなく1枚ずつ順に配る（本物の配り方に近づける）
        bool dealt = true;
        while(dealt) {
            dealt = false;
            for(size_t p = 0; p < session.match.Players().size(); ++p) {
                const auto&  hand  = session.match.Players()[p].hand;
                const size_t shown = session.board->HandSize(p);
                if(shown >= hand.size()) {
                    continue;
                }
                FrameMonitor::Mark("deal");
                const Tsukino::ECS::Entity entity = session.board->SpawnAtDeck(hand[shown]);
                session.board->AddToHand(p, entity);
                // 配る先の姿勢を先に決めてから飛ばす
                hlslpp::float3     position;
                hlslpp::quaternion rotation;
                float              scale = 1.0f;
                session.board->HandPose(p, shown, position, rotation, scale);
                ECS::CardMotion::SetTarget(registry, entity, position, rotation, scale);
                ECS::CardMotion::Fly(registry, entity, 0.5f, 70.0f, stagger);
                session.events->Publish(CardDealtEvent{stagger});
                last = stagger + 0.5f;
                stagger += 0.09f;
                dealt = true;
            }
        }
        return last;
    }

    //----------------------------------------------------------------------------
    //! 手番を始めます。
    //----------------------------------------------------------------------------
    void StartTurn(Tsukino::ECS::Registry& registry) {
        MatchSession& session = Session(registry);
        if(session.match.Outcome() != Rules::MatchOutcome::InProgress) {
            EndMatch(registry);
            return;
        }
        const size_t player = session.match.CurrentPlayerIndex();
        const bool   human  = !session.match.Players()[player].isCpu;
        // 人間なら入力待ちへ、CPU なら CpuTurnSystem が考え始める
        session.phase = human ? MatchPhase::HumanTurn : MatchPhase::CpuTurn;
        session.events->Publish(TurnStartedEvent{player, human});
    }

    //----------------------------------------------------------------------------
    //! 手札のカードを場に出し、公開と判定の演出を始めます。
    //----------------------------------------------------------------------------
    void CommitPlay(Tsukino::ECS::Registry& registry, size_t player, size_t handIndex, size_t slot) {
        MatchSession&      session = Session(registry);
        Board::BoardView&  board   = *session.board;
        Board::HandState&  hand    = board.Hand(player);
        if(handIndex >= board.HandSize(player)) {
            return;
        }

        const auto result = session.match.Play(handIndex, slot);
        if(!result) {
            return;
        }
        const Tsukino::ECS::Entity entity = board.TakeFromHand(player, handIndex);
        hand.hovered = -1;
        hand.lifted  = -1;

        session.phase         = MatchPhase::Resolving;
        session.input.preview = std::nullopt;
        board.SetRowHovered(-1);
        ECS::StopGlow(registry, entity);
        board.SetHeld(entity);
        board.SetGap(slot);
        board.SetMarker(false);

        // 置いた場所が端に畳まれているなら、そこまで列を流す
        board.ScrollToSlot(slot);

        FrameMonitor::Mark("place card");
        auto& reveal  = registry.AddComponent<ECS::RevealComponent>(entity);
        reveal.player = player;
        reveal.result = *result;
        session.events->Publish(CardCommittedEvent{player, entity});
    }

    //----------------------------------------------------------------------------
    //! 判定の演出のあと、手札を補充して手番を終えます。
    //----------------------------------------------------------------------------
    void FinishPlay(Tsukino::ECS::Registry& registry) {
        // 補充（ソロ）・ペナルティ（対戦）で引いたカードを配る
        const float wait = DealToHands(registry, 0.0f);
        Session(registry).Schedule(MatchStep::AdvanceTurn, wait + 0.15f);
    }

    //----------------------------------------------------------------------------
    //! 手番を次へ回します。
    //----------------------------------------------------------------------------
    void AdvanceTurn(Tsukino::ECS::Registry& registry) {
        MatchSession& session = Session(registry);
        if(session.match.Outcome() != Rules::MatchOutcome::InProgress) {
            EndMatch(registry);
            return;
        }
        session.match.AdvanceTurn();
        if(session.match.Outcome() != Rules::MatchOutcome::InProgress) {
            EndMatch(registry);
            return;
        }

        // 延長戦が始まったら知らせて、追加の1枚を配る
        float delay = 0.0f;
        if(session.match.IsTiebreak() && !session.wasTiebreak) {
            session.wasTiebreak = true;
            session.events->Publish(TiebreakStartedEvent{});
            delay = DealToHands(registry, 0.8f) + 0.2f;
        }
        session.Schedule(MatchStep::StartTurn, delay);
    }

    //----------------------------------------------------------------------------
    //! 対局を終え、記録を保存して結果の演出を始めます。
    //----------------------------------------------------------------------------
    void EndMatch(Tsukino::ECS::Registry& registry) {
        MatchSession& session = Session(registry);
        if(session.phase == MatchPhase::Finished) {
            return;
        }
        session.phase = MatchPhase::Finished;

        const Rules::MatchOutcome outcome = session.match.Outcome();
        const bool                won     = outcome == Rules::MatchOutcome::SoloCleared ||
                                            (outcome == Rules::MatchOutcome::VersusWon && session.match.Winner() == 0);

        // 記録を更新して保存する（ソロの記録は出題の範囲ごとに比べる）
        SaveData save      = LoadSaveData();
        bool     newRecord = false;
        if(session.config.mode == Rules::MatchMode::Solo) {
            SoloRecord& record = save.Record(session.config.range);
            newRecord          = session.match.Score() > record.bestScore && session.match.Score() > 0;
            record.bestScore   = std::max(record.bestScore, session.match.Score());
            record.bestPlaced  = std::max(record.bestPlaced, session.match.Players()[0].correctCount);
            record.bestStreak  = std::max(record.bestStreak, session.match.BestStreak());
        } else if(won) {
            ++save.versusWins;
        }
        StoreSaveData(save);

        session.events->Publish(MatchEndedEvent{outcome, won, newRecord});
        session.Schedule(MatchStep::ShowResult, 1.4f);
    }

    //----------------------------------------------------------------------------
    //! 人間の手番の選択とドラッグを解除し、カードを手札へ戻します。
    //----------------------------------------------------------------------------
    void CancelSelection(Tsukino::ECS::Registry& registry) {
        MatchSession&     session = Session(registry);
        Board::BoardView& board   = *session.board;
        HumanInput&       input   = session.input;
        const size_t      count   = board.HandSize(0);
        if(input.selected >= 0 && static_cast<size_t>(input.selected) < count) {
            ECS::StopGlow(registry, board.HandCard(0, static_cast<size_t>(input.selected)));
        }
        if(input.grabbed >= 0 && static_cast<size_t>(input.grabbed) < count) {
            ECS::CardMotion::SetSharpness(registry, board.HandCard(0, static_cast<size_t>(input.grabbed)), 14.0f, 10.0f);
        }
        input.selected       = -1;
        input.grabbed        = -1;
        input.dragging       = false;
        board.Hand(0).lifted = -1;
        board.SetHeld(entt::null);
        input.preview = std::nullopt;
        board.SetGap(std::nullopt);
        board.SetMarker(false);
    }

}    // namespace ChronoSort::Game
