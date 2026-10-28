//----------------------------------------------------------------------------
//! @file   MatchFeedbackSystem.cpp
//! @brief  対局の出来事に音・粒子・画面 UI で返すシステムの実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/System/MatchFeedbackSystem.hpp>

#include <ChronoSort/Board/BoardView.hpp>
#include <ChronoSort/ECS/CardMotion.hpp>
#include <ChronoSort/ECS/System/CameraRigSystem.hpp>
#include <ChronoSort/ECS/System/CardGlowSystem.hpp>
#include <ChronoSort/Effects/ParticleEmitter.hpp>
#include <ChronoSort/Game/FrameMonitor.hpp>
#include <ChronoSort/Game/GameServices.hpp>
#include <ChronoSort/Match/MatchSession.hpp>
#include <ChronoSort/Match/MatchEvents.hpp>
#include <ChronoSort/UI/GameHud.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/ECS/Event/EventBus.hpp>

namespace ChronoSort::ECS {

    namespace {
        const hlslpp::float3 kUp(0.0f, 1.0f, 0.0f);
        const hlslpp::float3 kGoldGlow(0.55f, 0.38f, 0.1f);
        const hlslpp::float3 kRedGlow(0.6f, 0.05f, 0.03f);
    }    // namespace

    //----------------------------------------------------------------------------
    //! 出来事の購読を始めます。
    //----------------------------------------------------------------------------
    MatchFeedbackSystem::MatchFeedbackSystem(Tsukino::ECS::Registry& registry, Tsukino::ECS::EventBus& events) {
        Tsukino::ECS::Registry* reg = &registry;
        auto sounds = []() -> Game::SoundBank& { return Game::Services().Sounds(); };
        auto session = [reg]() -> Match::MatchSession& { return Match::Session(*reg); };

        m_connections.push_back(events.Subscribe<Match::OpeningCardDealtEvent>([reg, sounds, session](const Match::OpeningCardDealtEvent& e) {
            // 山札から飛んで、降りたところで砂埃と金色の光
            Tsukino::Core::Sequencer& sequencer = session().sequencer;
            sequencer.Schedule(0.2f, [sounds] { sounds().Play("whoosh", 0.6f); });
            sequencer.Schedule(0.9f, [reg, sounds, session, card = e.card] {
                session().particles->Dust(reg->GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(card).position);
                Glow(*reg, card, kGoldGlow, 1.6f, 1.4f);
                sounds().Play("card_place");
            });
        }));
        m_connections.push_back(events.Subscribe<Match::CardDealtEvent>([sounds, session](const Match::CardDealtEvent& e) {
            session().sequencer.Schedule(e.delay, [sounds] { sounds().Play("deal", 0.55f); });
        }));
        m_connections.push_back(events.Subscribe<Match::TurnStartedEvent>([session](const Match::TurnStartedEvent& e) {
            Match::MatchSession& s = session();
            s.hud->SetTurn(e.player);
            if(e.human && s.config.mode == Rules::MatchMode::Versus) {
                s.hud->ShowBanner(L"あなたの番", false);
            }
        }));
        m_connections.push_back(events.Subscribe<Match::TiebreakStartedEvent>([sounds, session](const Match::TiebreakStartedEvent&) {
            session().hud->ShowBanner(L"延長戦！", true);
            sounds().Play("page");
        }));
        m_connections.push_back(events.Subscribe<Match::CpuThinkingStartedEvent>([sounds](const Match::CpuThinkingStartedEvent&) { sounds().Play("card_flip", 0.4f); }));
        m_connections.push_back(events.Subscribe<Match::CpuHoveredSlotEvent>([sounds](const Match::CpuHoveredSlotEvent&) { sounds().Play("tick", 0.3f); }));
        m_connections.push_back(events.Subscribe<Match::CardCommittedEvent>([sounds](const Match::CardCommittedEvent&) { sounds().Play("whoosh", 0.5f); }));
        m_connections.push_back(events.Subscribe<Match::CardFlippedEvent>([sounds](const Match::CardFlippedEvent&) { sounds().Play("card_flip"); }));
        m_connections.push_back(events.Subscribe<Match::CardLandedEvent>([reg, sounds, session](const Match::CardLandedEvent& e) {
            session().particles->Dust(e.at);
            ShakeCamera(*reg, 4.0f);
            sounds().Play("card_place");
        }));
        m_connections.push_back(events.Subscribe<Match::PlayJudgedEvent>([reg, sounds, session](const Match::PlayJudgedEvent& e) {
            Match::MatchSession& s = session();
            Game::FrameMonitor::Mark(e.result.correct ? "judge correct" : "judge wrong");
            s.hud->ShowStamp(e.card, e.result.correct);
            sounds().Play("stamp", 0.8f);
            if(e.result.correct) {
                Glow(*reg, e.card, kGoldGlow, 1.4f, 1.1f);
                s.particles->GoldBurst(e.at + kUp * 8.0f);
                sounds().Play("correct");
                if(s.config.mode == Rules::MatchMode::Solo) {
                    s.hud->ShowScorePopup(e.card, e.result.scoreGained, e.result.closeCall, e.result.streak);
                } else if(e.result.closeCall) {
                    s.hud->ShowScorePopup(e.card, 0, true, e.result.streak);
                }
            } else {
                // 不正解：赤く光って震える
                Glow(*reg, e.card, kRedGlow, 1.0f, 0.6f);
                CardMotion::Shake(*reg, e.card, 0.5f, 7.0f);
                s.particles->InkSplash(e.at + kUp * 6.0f);
                ShakeCamera(*reg, 7.0f);
                sounds().Play("wrong");
            }
            s.hud->OnPlayResolved(e.player, e.result);
        }));
        m_connections.push_back(events.Subscribe<Match::CorrectSlotShownEvent>([sounds](const Match::CorrectSlotShownEvent&) { sounds().Play("card_slide", 0.5f); }));
        m_connections.push_back(events.Subscribe<Match::CardDiscardedEvent>([sounds](const Match::CardDiscardedEvent&) { sounds().Play("whoosh", 0.6f); }));
        m_connections.push_back(events.Subscribe<Match::MatchEndedEvent>([reg, sounds, session](const Match::MatchEndedEvent& e) {
            Match::MatchSession& s = session();
            sounds().Play(e.won ? "fanfare" : "lose");
            if(s.config.mode == Rules::MatchMode::Solo) {
                s.hud->SetNewRecord(e.newRecord);
            }
            s.hud->ShowBanner(e.won ? L"見事！" : (e.outcome == Rules::MatchOutcome::SoloFailed ? L"インクが尽きた…" : L"惜しい…"), true);
            if(e.won) {
                // 場の列のカードを左から順に光らせる
                const std::vector<Tsukino::ECS::Entity> row = s.board->Row();
                for(size_t i = 0; i < row.size(); ++i) {
                    s.sequencer.Schedule(0.05f * static_cast<float>(i), [reg, entity = row[i]] {
                        Glow(*reg, entity, kGoldGlow, 1.5f, 1.2f);
                        CardMotion::Punch(*reg, entity, 0.3f, 0.06f);
                    });
                }
            }
        }));
        m_connections.push_back(events.Subscribe<Match::ResultRequestedEvent>([sounds, session](const Match::ResultRequestedEvent&) {
            Game::FrameMonitor::Mark("show result");
            session().hud->ShowResult();
            sounds().Play("page");
            sounds().PlayBgm("result");
        }));
    }

    //----------------------------------------------------------------------------
    //! 何もしません（出来事を受けたときにだけ動く）。
    //----------------------------------------------------------------------------
    void MatchFeedbackSystem::Update(Tsukino::ECS::Registry&, float) {
    }

}    // namespace ChronoSort::ECS
