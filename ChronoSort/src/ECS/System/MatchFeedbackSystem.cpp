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
#include <ChronoSort/Game/TuningSet.hpp>
#include <ChronoSort/Util/TextUtil.hpp>
#include <ChronoSort/Match/MatchSession.hpp>
#include <ChronoSort/Match/MatchEvents.hpp>
#include <ChronoSort/UI/GameHud.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/ECS/Event/EventBus.hpp>

namespace ChronoSort::ECS {

    namespace {
        const hlslpp::float3 kUp(0.0f, 1.0f, 0.0f);
    }    // namespace

    //----------------------------------------------------------------------------
    //! 出来事の購読を始めます。
    //----------------------------------------------------------------------------
    MatchFeedbackSystem::MatchFeedbackSystem(Tsukino::ECS::Registry& registry, Tsukino::ECS::EventBus& events) {
        Tsukino::ECS::Registry* reg = &registry;
        auto sounds = []() -> Game::SoundBank& { return Game::Services().Sounds(); };
        auto session = [reg]() -> Match::MatchSession& { return Match::Session(*reg); };
        auto tuning  = []() -> const Match::FeedbackTuning& { return Game::Tuning().Feedback(); };

        m_connections.push_back(events.Subscribe<Match::OpeningCardDealtEvent>([reg, sounds, session, tuning](const Match::OpeningCardDealtEvent& e) {
            // 山札から飛んで、降りたところで砂埃と金色の光
            Tsukino::Core::Sequencer& sequencer = session().sequencer;
            sequencer.Schedule(tuning().openingWhooshDelay, [sounds] { sounds().Play("deal.openingFly"); });
            sequencer.Schedule(tuning().openingLandDelay, [reg, sounds, session, tuning, card = e.card] {
                session().particles->Emit("dust", reg->GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(card).position);
                Glow(*reg, card, tuning().openingGlow);
                sounds().Play("deal.openingLand");
            });
        }));
        m_connections.push_back(events.Subscribe<Match::CardDealtEvent>([sounds, session](const Match::CardDealtEvent& e) {
            session().sequencer.Schedule(e.delay, [sounds] { sounds().Play("deal.card"); });
        }));
        m_connections.push_back(events.Subscribe<Match::TurnStartedEvent>([session, tuning](const Match::TurnStartedEvent& e) {
            Match::MatchSession& s = session();
            s.hud->SetTurn(e.player);
            if(e.human && s.config.mode == Rules::MatchMode::Versus) {
                s.hud->ShowBanner(Util::Utf8ToWide(tuning().bannerYourTurn), false);
            }
        }));
        m_connections.push_back(events.Subscribe<Match::TiebreakStartedEvent>([sounds, session, tuning](const Match::TiebreakStartedEvent&) {
            session().hud->ShowBanner(Util::Utf8ToWide(tuning().bannerTiebreak), true);
            sounds().Play("match.tiebreak");
        }));
        m_connections.push_back(events.Subscribe<Match::CpuThinkingStartedEvent>([sounds](const Match::CpuThinkingStartedEvent&) { sounds().Play("cpu.pick"); }));
        m_connections.push_back(events.Subscribe<Match::CpuHoveredSlotEvent>([sounds](const Match::CpuHoveredSlotEvent&) { sounds().Play("cpu.hover"); }));
        m_connections.push_back(events.Subscribe<Match::CardCommittedEvent>([sounds](const Match::CardCommittedEvent&) { sounds().Play("play.commit"); }));
        m_connections.push_back(events.Subscribe<Match::CardFlippedEvent>([sounds](const Match::CardFlippedEvent&) { sounds().Play("play.flip"); }));
        m_connections.push_back(events.Subscribe<Match::CardLandedEvent>([reg, sounds, session, tuning](const Match::CardLandedEvent& e) {
            session().particles->Emit("dust", e.at);
            ShakeCamera(*reg, tuning().landShake);
            sounds().Play("play.land");
        }));
        m_connections.push_back(events.Subscribe<Match::PlayJudgedEvent>([reg, sounds, session, tuning](const Match::PlayJudgedEvent& e) {
            Match::MatchSession&         s = session();
            const Match::FeedbackTuning& t = tuning();
            Game::FrameMonitor::Mark(e.result.correct ? "judge correct" : "judge wrong");
            s.hud->ShowStamp(e.card, e.result.correct);
            sounds().Play("play.stamp");
            if(e.result.correct) {
                Glow(*reg, e.card, t.correctGlow);
                s.particles->Emit("gold", e.at + kUp * t.correctBurstHeight);
                sounds().Play("play.correct");
                if(s.config.mode == Rules::MatchMode::Solo) {
                    s.hud->ShowScorePopup(e.card, e.result.scoreGained, e.result.closeCall, e.result.streak);
                } else if(e.result.closeCall) {
                    s.hud->ShowScorePopup(e.card, 0, true, e.result.streak);
                }
            } else {
                // 不正解：赤く光って震える
                Glow(*reg, e.card, t.wrongGlow);
                CardMotion::Shake(*reg, e.card, t.wrongTremor);
                s.particles->Emit("ink", e.at + kUp * t.wrongBurstHeight);
                ShakeCamera(*reg, t.wrongShake);
                sounds().Play("play.wrong");
            }
            s.hud->OnPlayResolved(e.player, e.result);
        }));
        m_connections.push_back(events.Subscribe<Match::CorrectSlotShownEvent>([sounds](const Match::CorrectSlotShownEvent&) { sounds().Play("play.showCorrect"); }));
        m_connections.push_back(events.Subscribe<Match::CardDiscardedEvent>([sounds](const Match::CardDiscardedEvent&) { sounds().Play("play.discard"); }));
        m_connections.push_back(events.Subscribe<Match::MatchEndedEvent>([reg, sounds, session, tuning](const Match::MatchEndedEvent& e) {
            Match::MatchSession&         s = session();
            const Match::FeedbackTuning& t = tuning();
            sounds().Play(e.won ? "match.won" : "match.lost");
            if(s.config.mode == Rules::MatchMode::Solo) {
                s.hud->SetNewRecord(e.newRecord);
            }
            s.hud->ShowBanner(Util::Utf8ToWide(e.won ? t.bannerWon : (e.outcome == Rules::MatchOutcome::SoloFailed ? t.bannerInkOut : t.bannerLost)), true);
            if(e.won) {
                // 場の列のカードを左から順に光らせる
                const std::vector<Tsukino::ECS::Entity> row = s.board->Row();
                for(size_t i = 0; i < row.size(); ++i) {
                    s.sequencer.Schedule(t.winWaveStep * static_cast<float>(i), [reg, tuning, entity = row[i]] {
                        Glow(*reg, entity, tuning().winGlow);
                        CardMotion::Punch(*reg, entity, tuning().winBump);
                    });
                }
            }
        }));
        m_connections.push_back(events.Subscribe<Match::ResultRequestedEvent>([sounds, session](const Match::ResultRequestedEvent&) {
            Game::FrameMonitor::Mark("show result");
            session().hud->ShowResult();
            sounds().Play("result.show");
            sounds().PlayBgm("result");
        }));
    }

    //----------------------------------------------------------------------------
    //! 何もしません（出来事を受けたときにだけ動く）。
    //----------------------------------------------------------------------------
    void MatchFeedbackSystem::Update(Tsukino::ECS::Registry&, float) {
    }

}    // namespace ChronoSort::ECS
