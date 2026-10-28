//----------------------------------------------------------------------------
//! @file   RevealSystem.cpp
//! @brief  出したカードの公開と判定の演出を進めるシステムの実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/System/RevealSystem.hpp>

#include <ChronoSort/Board/BoardView.hpp>
#include <ChronoSort/ECS/CardMotion.hpp>
#include <ChronoSort/ECS/Component/RevealComponent.hpp>
#include <ChronoSort/Match/MatchActions.hpp>
#include <ChronoSort/Match/MatchEvents.hpp>

#include <Tsukino/Core/ECS/Event/EventBus.hpp>

#include <vector>

namespace ChronoSort::ECS {

    namespace {
        // 公開の演出の時刻（出してからの秒）。正解は判定から 0.8 秒、不正解は正しい位置を見せてから捨て札へ
        constexpr float kFlipAt        = 0.42f;
        constexpr float kSlamAt        = 0.98f;
        constexpr float kLandAt        = 1.10f;
        constexpr float kJudgeAt       = 1.32f;
        constexpr float kCorrectDoneAt = kJudgeAt + 0.8f;
        constexpr float kShowCorrectAt = kJudgeAt + 0.75f;
        constexpr float kDiscardAt     = kJudgeAt + 1.9f;
        constexpr float kWrongDoneAt   = kJudgeAt + 2.4f;

        const hlslpp::float3 kUp(0.0f, 1.0f, 0.0f);

        //----------------------------------------------------------------------------
        //! 公開した面（年号の側）を、列と同じ角度だけカメラへ起こした向きを返します。
        //! @return 向き。
        //----------------------------------------------------------------------------
        hlslpp::quaternion FacingRevealed() {
            return hlslpp::mul(hlslpp::quaternion::rotation_x(-Board::kRowTilt), Board::RevealedRotation());
        }

        //----------------------------------------------------------------------------
        //! 公開の演出を1段階進めます。
        //! @param  [in,out] registry レジストリ。
        //! @param  [in,out] session  対局の状態。
        //! @param  [in]     entity   出したカード。
        //! @param  [in,out] reveal   演出の進み具合。
        //! @return 次の段階の時刻が来ていれば、続けて進めてよいなら true。
        //----------------------------------------------------------------------------
        bool StepReveal(Tsukino::ECS::Registry& registry, Match::MatchSession& session, Tsukino::ECS::Entity entity, RevealComponent& reveal) {
            Board::BoardView&        board  = *session.board;
            const Rules::PlayResult& result = reveal.result;
            const size_t             slot   = result.slot;
            const size_t             correctSlot = result.correctSlots.empty() ? result.slot : result.correctSlots.front();

            switch(reveal.stage) {
            case RevealStage::Carry:
                // 1. 隙間の上まで運ぶ（表のまま）。出したその場で行い、時間は次のフレームから数える
                CardMotion::SetTarget(registry, entity, board.SlotPosition(slot) + kUp * 60.0f, hlslpp::quaternion::identity(), 1.0f);
                CardMotion::Fly(registry, entity, 0.38f, 35.0f);
                reveal.stage = RevealStage::Flip;
                return false;

            case RevealStage::Flip:
                // 2. 持ち上げながら裏返す
                if(reveal.time < kFlipAt) {
                    return false;
                }
                CardMotion::SetSharpness(registry, entity, 9.0f, 7.0f);
                CardMotion::SetTarget(registry, entity, board.SlotPosition(slot) + kUp * 115.0f, FacingRevealed(), board.SlotScale(slot) * 1.1f);
                session.events->Publish(Match::CardFlippedEvent{entity});
                reveal.stage = RevealStage::Slam;
                return true;

            case RevealStage::Slam: {
                // 3. 卓へ叩きつける。着地先は列に並んだときと同じ起こした姿勢（寝てから起き直す二段の動きを出さない）
                if(reveal.time < kSlamAt) {
                    return false;
                }
                CardMotion::SetSharpness(registry, entity, 34.0f, 16.0f);
                hlslpp::float3     position;
                hlslpp::quaternion rotation;
                float              scale = 1.0f;
                board.SlotRestPose(slot, position, rotation, scale);
                CardMotion::SetTarget(registry, entity, position, rotation, scale);
                reveal.stage = RevealStage::Land;
                return true;
            }

            case RevealStage::Land:
                if(reveal.time < kLandAt) {
                    return false;
                }
                CardMotion::Punch(registry, entity, 0.3f, 0.07f);
                session.events->Publish(Match::CardLandedEvent{entity, board.SlotPosition(slot)});
                reveal.stage = RevealStage::Judge;
                return true;

            case RevealStage::Judge: {
                // 4. 判定。正解なら列に入れ、不正解なら正しい位置を見せてから捨て札へ
                if(reveal.time < kJudgeAt) {
                    return false;
                }
                CardMotion::SetSharpness(registry, entity, 14.0f, 10.0f);
                const hlslpp::float3 at = board.SlotPosition(slot);
                if(result.correct) {
                    board.InsertIntoRow(slot, entity);
                    board.SetHeld(entt::null);
                }
                session.events->Publish(Match::PlayJudgedEvent{reveal.player, entity, result, at});
                reveal.stage = result.correct ? RevealStage::Finish : RevealStage::ShowCorrect;
                return true;
            }

            case RevealStage::ShowCorrect:
                if(reveal.time < kShowCorrectAt) {
                    return false;
                }
                board.SetGap(correctSlot);
                board.SetMarker(true, true);
                board.ScrollToSlot(correctSlot);
                CardMotion::SetSharpness(registry, entity, 8.0f, 8.0f);
                CardMotion::SetTarget(registry, entity, board.SlotPosition(correctSlot) + kUp * 40.0f, FacingRevealed(), board.SlotScale(correctSlot));
                session.events->Publish(Match::CorrectSlotShownEvent{entity});
                reveal.stage = RevealStage::Discard;
                return true;

            case RevealStage::Discard:
                if(reveal.time < kDiscardAt) {
                    return false;
                }
                board.SetGap(std::nullopt);
                board.SetMarker(false);
                board.SetHeld(entt::null);
                CardMotion::SetSharpness(registry, entity, 14.0f, 10.0f);
                board.SendToDiscard(entity);
                session.events->Publish(Match::CardDiscardedEvent{entity});
                reveal.stage = RevealStage::Finish;
                return true;

            case RevealStage::Finish:
                if(reveal.time < (result.correct ? kCorrectDoneAt : kWrongDoneAt)) {
                    return false;
                }
                reveal.stage = RevealStage::Done;
                return false;

            default:
                return false;
            }
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! 出したカードの公開と判定の演出を進めます。
    //----------------------------------------------------------------------------
    void RevealSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        if(!Match::HasSession(registry)) {
            return;
        }
        Match::MatchSession& session = Match::Session(registry);
        if(session.paused) {
            return;
        }

        // 演出の中で列に入れたり外したりするので、先に対象を集めてから進める
        std::vector<Tsukino::ECS::Entity> revealing;
        registry.View<RevealComponent>().each([&](Tsukino::ECS::Entity entity, RevealComponent&) { revealing.push_back(entity); });

        for(const Tsukino::ECS::Entity entity : revealing) {
            RevealComponent& reveal = registry.GetComponent<RevealComponent>(entity);
            if(reveal.stage != RevealStage::Carry) {
                reveal.time += deltaTime;
            }
            while(StepReveal(registry, session, entity, reveal)) {
            }
            if(reveal.stage == RevealStage::Done) {
                registry.RemoveComponent<RevealComponent>(entity);
                Match::FinishPlay(registry);
            }
        }
    }

}    // namespace ChronoSort::ECS
