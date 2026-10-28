//----------------------------------------------------------------------------
//! @file   MatchSystems.cpp
//! @brief  対局を進めるシステムの実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/System/MatchSystems.hpp>

#include <ChronoSort/Board/BoardView.hpp>
#include <ChronoSort/ECS/CardMotion.hpp>
#include <ChronoSort/ECS/Component/CameraRigComponent.hpp>
#include <ChronoSort/ECS/Component/RevealComponent.hpp>
#include <ChronoSort/ECS/System/CardGlowSystem.hpp>
#include <ChronoSort/Effects/ParticleEmitter.hpp>
#include <ChronoSort/Game/FrameMonitor.hpp>
#include <ChronoSort/Game/MatchActions.hpp>
#include <ChronoSort/Game/MatchEvents.hpp>
#include <ChronoSort/Game/Options.hpp>
#include <ChronoSort/Game/Sounds.hpp>
#include <ChronoSort/UI/GameHud.hpp>

#include <Tsukino/BuiltIn/ECS/Camera/CameraUtility.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/ECS/Event/EventBus.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Window.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <algorithm>
#include <cmath>

namespace ChronoSort::ECS {

    namespace {
        namespace CameraUtility = Tsukino::BuiltIn::ECS::CameraUtility;
        using Tsukino::Input::KeyCode;

        // 人間の入力
        constexpr float kClickThreshold = 8.0f;      // これ以上動いたらクリックではなくドラッグ
        constexpr float kRowBandNear    = 130.0f;    // 場の列の手前、この距離までなら「列に置こうとしている」
        constexpr float kRowBandFar     = 160.0f;    // 場の列の奥、この距離まで
        constexpr float kEdgePanMargin  = 0.08f;     // 画面端のこの割合の幅に入ると場の列が流れる
        constexpr float kEdgePanSpeed   = 420.0f;    // 画面端で列を流す速さ
        constexpr float kWheelScroll    = 60.0f;     // ホイール1段で列を流す量（ホイールの値は段数で来る）

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
        const hlslpp::float3 kGoldGlow(0.55f, 0.38f, 0.1f);
        const hlslpp::float3 kRedGlow(0.6f, 0.05f, 0.03f);
        const hlslpp::float3 kSelectGlow(0.22f, 0.16f, 0.05f);

        //----------------------------------------------------------------------------
        //! 公開した面（年号の側）を、列と同じ角度だけカメラへ起こした向きを返します。
        //! @return 向き。
        //----------------------------------------------------------------------------
        hlslpp::quaternion FacingRevealed() {
            return hlslpp::mul(hlslpp::quaternion::rotation_x(-Board::kRowTilt), Board::RevealedRotation());
        }

        //----------------------------------------------------------------------------
        //! エンジンのコンテキストを返します。
        //! @param  [in] registry レジストリ。
        //! @return コンテキスト。
        //----------------------------------------------------------------------------
        Tsukino::EngineIntegration::EngineContext& Engine(Tsukino::ECS::Registry& registry) {
            return *registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        }

        //----------------------------------------------------------------------------
        //! マウスのレイを返します。
        //! @param  [in] registry レジストリ。
        //! @param  [in] camera   卓を見下ろすカメラ。
        //! @return ワールド空間のレイ。
        //----------------------------------------------------------------------------
        CameraUtility::Ray MouseRay(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity camera) {
            auto&       context = Engine(registry);
            const auto& cam     = registry.GetComponent<Tsukino::BuiltIn::ECS::CameraComponent>(camera);
            int         x       = 0;
            int         y       = 0;
            context.inputSystem->GetMousePosition(&x, &y);
            const hlslpp::float2 size(static_cast<float>(context.window->GetWidth()), static_cast<float>(context.window->GetHeight()));
            return CameraUtility::ScreenPointToRay(cam, hlslpp::float2(static_cast<float>(x), static_cast<float>(y)), size);
        }

        //----------------------------------------------------------------------------
        //! 人間の手番の、手札と場の列への入力を処理します。
        //! @param  [in,out] registry  レジストリ。
        //! @param  [in,out] session   対局の状態。
        //! @param  [in]     deltaTime 経過時間（秒）。
        //----------------------------------------------------------------------------
        void UpdateHandInput(Tsukino::ECS::Registry& registry, Game::MatchSession& session, float deltaTime) {
            auto&                        context = Engine(registry);
            Tsukino::Input::InputSystem& keys    = *context.inputSystem;
            Board::BoardView&            board   = *session.board;
            Board::HandState&            hand    = board.Hand(0);
            Game::HumanInput&            input   = session.input;
            Game::SoundBank&             sounds  = Game::Sounds(context);

            int mouseX = 0;
            int mouseY = 0;
            keys.GetMousePosition(&mouseX, &mouseY);
            const hlslpp::float2 mouse(static_cast<float>(mouseX), static_cast<float>(mouseY));
            const auto           ray = MouseRay(registry, session.camera);

            // カーソルが重なっている手札（ドラッグ中・選択中は更新しない）
            if(!input.dragging && input.selected < 0) {
                hlslpp::float2 uv;
                const int      hovered = board.PickHandCard(0, ray, uv);
                if(hovered != hand.hovered && hovered >= 0) {
                    Game::FrameMonitor::Mark("hover hand card");
                    sounds.Play("card_slide", 0.25f);
                }
                hand.hovered   = hovered;
                hand.hoverTilt = hovered >= 0 ? hlslpp::float2((float(uv.x) - 0.5f) * 2.0f, (float(uv.y) - 0.5f) * 2.0f) : hlslpp::float2(0.0f, 0.0f);
            }

            // 手札を持っていないときは、指した場のカードを起こして年号を読めるようにする
            board.SetRowHovered((!input.dragging && input.selected < 0 && hand.hovered < 0) ? board.PickRowCard(ray) : -1);

            // 列の上のカーソル（持っているカード）の位置を注目点にして、周りのカードを読める大きさに広げる
            {
                std::optional<float> focus;
                hlslpp::float3       point    = input.dragPoint;
                float                distance = 0.0f;
                const bool           onTable  = input.dragging || CameraUtility::IntersectPlane(ray, hlslpp::float3(0.0f, 0.0f, 0.0f), hlslpp::float3(0.0f, 1.0f, 0.0f), distance);
                if(!input.dragging && onTable) {
                    point = ray.GetPoint(distance);
                }
                if(onTable && float(point.z) >= Board::kRowZ - kRowBandNear && float(point.z) <= Board::kRowZ + kRowBandFar) {
                    focus = float(point.x);
                }
                board.SetFocusX(focus);
            }

            // 置き先の候補：卓の上のカーソル位置が場の列の近くならスロットを決める
            auto previewFrom = [&](const hlslpp::float3& point) -> std::optional<size_t> {
                if(float(point.z) < Board::kRowZ - kRowBandNear || float(point.z) > Board::kRowZ + kRowBandFar) {
                    return std::nullopt;
                }
                return board.SlotFromWorldX(point.x);
            };
            auto setPreview = [&](std::optional<size_t> slot) {
                if(slot != input.preview) {
                    if(slot) {
                        sounds.Play("tick", 0.35f);
                    }
                    input.preview = slot;
                    board.SetGap(slot);
                    board.SetMarker(slot.has_value());
                }
            };

            // 押した
            if(keys.IsKeyPressed(KeyCode::LButton)) {
                if(input.selected >= 0) {
                    // 選択中：列の上なら置く、それ以外なら選び直し
                    if(input.preview) {
                        const size_t slot = *input.preview;
                        const int    card = input.selected;
                        input.selected    = -1;
                        hand.lifted       = -1;
                        Game::CommitPlay(registry, 0, static_cast<size_t>(card), slot);
                        return;
                    }
                    const int previous = input.selected;
                    Game::CancelSelection(registry);
                    if(hand.hovered >= 0 && hand.hovered != previous) {
                        input.grabbed    = hand.hovered;
                        input.pressPoint = mouse;
                    }
                } else if(hand.hovered >= 0) {
                    input.grabbed    = hand.hovered;
                    input.pressPoint = mouse;
                    input.dragging   = false;
                }
            }

            // 押したまま動かした：ドラッグ開始・継続
            if(input.grabbed >= 0 && keys.IsKeyDown(KeyCode::LButton)) {
                const Tsukino::ECS::Entity entity = board.HandCard(0, static_cast<size_t>(input.grabbed));
                if(!input.dragging && float(hlslpp::length(mouse - input.pressPoint)) > kClickThreshold) {
                    input.dragging = true;
                    hand.hovered   = -1;
                    board.SetHeld(entity);
                    CardMotion::SetSharpness(registry, entity, 28.0f, 16.0f);
                    sounds.Play("card_slide", 0.6f);
                }
                if(input.dragging) {
                    float distance = 0.0f;
                    if(CameraUtility::IntersectPlane(ray, hlslpp::float3(0.0f, Board::kDragHeight, 0.0f), hlslpp::float3(0.0f, 1.0f, 0.0f), distance)) {
                        const hlslpp::float3 point = ray.GetPoint(distance);
                        // 速度は少しなまらせてから傾きに使う（手ぶれで暴れないように）
                        const hlslpp::float3 velocity = (point - input.dragPoint) / std::max(deltaTime, 0.001f);
                        input.dragVelocity += (velocity - input.dragVelocity) * std::min(1.0f, deltaTime * 12.0f);
                        input.dragPoint = point;

                        const float              roll  = std::clamp(-float(input.dragVelocity.x) * 0.0011f, -0.4f, 0.4f);
                        const float              pitch = std::clamp(float(input.dragVelocity.z) * 0.0011f, -0.4f, 0.4f);
                        const hlslpp::quaternion lean  = hlslpp::mul(hlslpp::quaternion::rotation_x(pitch), hlslpp::quaternion::rotation_z(roll));
                        CardMotion::SetTarget(registry, entity, point, lean, 1.05f);

                        setPreview(previewFrom(hlslpp::float3(point.x, 0.0f, point.z)));

                        // 画面端（列を畳んで重ねているあたり）に寄せたら、列を流して続きを見せる
                        const float width  = static_cast<float>(context.window->GetWidth());
                        const float margin = width * kEdgePanMargin;
                        if(float(mouse.x) < margin) {
                            board.ScrollBy(-kEdgePanSpeed * deltaTime);
                        } else if(float(mouse.x) > width - margin) {
                            board.ScrollBy(kEdgePanSpeed * deltaTime);
                        }
                    }
                }
            }

            // 離した
            if(input.grabbed >= 0 && keys.IsKeyReleased(KeyCode::LButton)) {
                const int index = input.grabbed;
                input.grabbed   = -1;
                if(input.dragging) {
                    input.dragging = false;
                    if(input.preview) {
                        Game::CommitPlay(registry, 0, static_cast<size_t>(index), *input.preview);
                        return;
                    }
                    // 置けない所で離した：手札へ放物線で戻す
                    const Tsukino::ECS::Entity entity = board.HandCard(0, static_cast<size_t>(index));
                    board.SetHeld(entt::null);
                    CardMotion::SetSharpness(registry, entity, 14.0f, 10.0f);
                    board.ApplyLayoutNow();
                    CardMotion::Fly(registry, entity, 0.35f, 30.0f);
                    sounds.Play("card_slide", 0.4f);
                } else {
                    // クリック：選択して掲げる（ドラッグが苦手でも置けるように）
                    input.selected = index;
                    hand.lifted    = index;
                    hand.hovered   = -1;
                    PulseGlow(registry, board.HandCard(0, static_cast<size_t>(index)), kSelectGlow, 1.0f, 5.0f);
                    sounds.Play("card_flip", 0.4f);
                }
            }

            // 選択中は、卓の上のカーソル位置で置き先を予告する
            if(input.selected >= 0 && !input.dragging) {
                float distance = 0.0f;
                if(CameraUtility::IntersectPlane(ray, hlslpp::float3(0.0f, 0.0f, 0.0f), hlslpp::float3(0.0f, 1.0f, 0.0f), distance)) {
                    setPreview(previewFrom(ray.GetPoint(distance)));
                }
                if(keys.IsKeyPressed(KeyCode::RButton)) {
                    Game::CancelSelection(registry);
                }
            } else if(!input.dragging && input.grabbed < 0) {
                setPreview(std::nullopt);
            }
        }

        //----------------------------------------------------------------------------
        //! 公開の演出を1段階進めます。
        //! @param  [in,out] registry レジストリ。
        //! @param  [in,out] session  対局の状態。
        //! @param  [in]     entity   出したカード。
        //! @param  [in,out] reveal   演出の進み具合。
        //! @return 次の段階の時刻が来ていれば、続けて進めてよいなら true。
        //----------------------------------------------------------------------------
        bool StepReveal(Tsukino::ECS::Registry& registry, Game::MatchSession& session, Tsukino::ECS::Entity entity, RevealComponent& reveal) {
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
                session.events->Publish(Game::CardFlippedEvent{entity});
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
                session.events->Publish(Game::CardLandedEvent{entity, board.SlotPosition(slot)});
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
                session.events->Publish(Game::PlayJudgedEvent{reveal.player, entity, result, at});
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
                session.events->Publish(Game::CorrectSlotShownEvent{entity});
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
                session.events->Publish(Game::CardDiscardedEvent{entity});
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
    //! 時間を置いた段取りと、見た目だけの時間差を進めます。
    //----------------------------------------------------------------------------
    void MatchFlowSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        if(!Game::HasSession(registry)) {
            return;
        }
        Game::MatchSession& session = Game::Session(registry);
        if(session.paused) {
            return;
        }
        session.sequencer.Update(deltaTime);

        if(session.step == Game::MatchStep::None) {
            return;
        }
        session.stepTimer -= deltaTime;
        if(session.stepTimer > 0.0f) {
            return;
        }
        // 段取りの中から次の段取りを予約できるよう、先に空けてから行う
        const Game::MatchStep step = session.step;
        session.step               = Game::MatchStep::None;
        switch(step) {
        case Game::MatchStep::StartTurn:
            Game::StartTurn(registry);
            break;
        case Game::MatchStep::AdvanceTurn:
            Game::AdvanceTurn(registry);
            break;
        case Game::MatchStep::ShowResult:
            session.events->Publish(Game::ResultRequestedEvent{});
            break;
        default:
            break;
        }
    }

    //----------------------------------------------------------------------------
    //! CPU の手番を進めます。
    //----------------------------------------------------------------------------
    void CpuTurnSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        if(!Game::HasSession(registry)) {
            return;
        }
        Game::MatchSession& session = Game::Session(registry);
        Game::CpuThinking&  cpu     = session.cpu;
        if(session.paused || session.phase != Game::MatchPhase::CpuTurn) {
            return;
        }

        if(!cpu.active) {
            // 手に取って考え始める。自信が無いほど長く迷い、途中で置き先の候補を行き来する
            cpu        = Game::CpuThinking{};
            cpu.active = true;
            cpu.player = session.match.CurrentPlayerIndex();
            cpu.move   = Rules::ChooseCpuMove(session.match.Players()[cpu.player], session.match.GetTimeline(), session.rng);

            std::uniform_real_distribution<float> unit(0.0f, 1.0f);
            cpu.think = 0.9f + (1.0f - cpu.move.confidence) * 1.6f + unit(session.rng) * 0.5f;

            const size_t slotCount = session.match.GetTimeline().SlotCount();
            const int    hops      = 1 + static_cast<int>((1.0f - cpu.move.confidence) * 3.0f);
            for(int i = 0; i < hops; ++i) {
                const float  at        = cpu.think * (0.35f + 0.5f * static_cast<float>(i) / static_cast<float>(hops));
                const int    wander    = std::uniform_int_distribution<int>(-1, 1)(session.rng);
                const size_t candidate = static_cast<size_t>(std::clamp(static_cast<int>(cpu.move.slot) + wander, 0, static_cast<int>(slotCount) - 1));
                cpu.hops.emplace_back(at, candidate);
            }
            session.board->Hand(cpu.player).lifted = static_cast<int>(cpu.move.handIndex);
            session.events->Publish(Game::CpuThinkingStartedEvent{cpu.player});
            return;
        }

        cpu.time += deltaTime;
        while(cpu.nextHop < cpu.hops.size() && cpu.hops[cpu.nextHop].first <= cpu.time) {
            const size_t slot = cpu.hops[cpu.nextHop].second;
            session.board->SetGap(slot);
            session.board->SetMarker(true);
            session.events->Publish(Game::CpuHoveredSlotEvent{slot});
            ++cpu.nextHop;
        }
        if(cpu.time >= cpu.think) {
            cpu.active                             = false;
            session.board->Hand(cpu.player).lifted = -1;
            Game::CommitPlay(registry, cpu.player, cpu.move.handIndex, cpu.move.slot);
        }
    }

    //----------------------------------------------------------------------------
    //! 人間の入力を処理します。
    //----------------------------------------------------------------------------
    void HumanInputSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        if(!Game::HasSession(registry)) {
            return;
        }
        Game::MatchSession&          session = Game::Session(registry);
        Tsukino::Input::InputSystem& keys    = *Engine(registry).inputSystem;

        // 人間の手番でなければ、カーソルではなく隙間（CPU の迷い・公開中）を注目点にする
        if(session.phase != Game::MatchPhase::HumanTurn) {
            session.board->SetFocusX(std::nullopt);
        }

        if(session.phase == Game::MatchPhase::HumanTurn && !session.paused) {
            UpdateHandInput(registry, session, deltaTime);
#ifdef _DEBUG
            // 確認用：F8 で人間の手番を CPU（やさしい）に代わりに打たせる
            if(session.phase == Game::MatchPhase::HumanTurn && keys.IsKeyPressed(KeyCode::F8)) {
                Game::CancelSelection(registry);
                Rules::Player stand = session.match.Players()[0];
                stand.cpuLevel      = Rules::CpuLevel::Easy;
                const Rules::CpuMove move = Rules::ChooseCpuMove(stand, session.match.GetTimeline(), session.rng);
                Game::CommitPlay(registry, 0, move.handIndex, move.slot);
            }
#endif
        }

        // ホイールで場の列を左右に見渡す（CPU の手番や公開の演出中も見られるよう、手番を問わず受け付ける）
        session.board->ScrollBy(-keys.GetWheelDelta() * kWheelScroll);

        // 人間の手番で操作していない時間（マウスを動かす・押す・ホイール・カードを持つ、で数え直す）
        Game::HumanInput& input  = session.input;
        int               mouseX = 0;
        int               mouseY = 0;
        keys.GetMousePosition(&mouseX, &mouseY);
        const hlslpp::float2 mouse(static_cast<float>(mouseX), static_cast<float>(mouseY));
        const bool           active = float(hlslpp::length(mouse - input.lastMouse)) > 2.0f || keys.IsKeyDown(KeyCode::LButton) ||
                                      keys.IsKeyDown(KeyCode::RButton) || keys.GetWheelDelta() != 0.0f || input.grabbed >= 0 || input.selected >= 0;
        input.lastMouse = mouse;
        input.idleTime  = (session.phase == Game::MatchPhase::HumanTurn && !active && !session.paused) ? input.idleTime + deltaTime : 0.0f;
        session.hud->SetIdleSeconds(input.idleTime);
    }

    //----------------------------------------------------------------------------
    //! 出したカードの公開と判定の演出を進めます。
    //----------------------------------------------------------------------------
    void RevealSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        if(!Game::HasSession(registry)) {
            return;
        }
        Game::MatchSession& session = Game::Session(registry);
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
                Game::FinishPlay(registry);
            }
        }
    }

    //----------------------------------------------------------------------------
    //! 卓のカメラに、揺れと漂いを足します。
    //----------------------------------------------------------------------------
    void CameraRigSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        registry.View<Tsukino::BuiltIn::ECS::TransformComponent, Tsukino::BuiltIn::ECS::CameraComponent, CameraRigComponent>().each(
            [deltaTime](auto, Tsukino::BuiltIn::ECS::TransformComponent& transform, Tsukino::BuiltIn::ECS::CameraComponent& camera, CameraRigComponent& rig) {
                rig.time += deltaTime;
                // 揺れは減衰するランダムなずれ
                rig.shake = std::max(0.0f, rig.shake - deltaTime * 18.0f);
                const hlslpp::float3 jolt(std::sin(rig.time * 71.0f) * rig.shake, std::cos(rig.time * 53.0f) * rig.shake * 0.6f, 0.0f);
                // ゆっくり呼吸するように少しだけ漂わせ、止まった絵に見せない
                const hlslpp::float3 drift(std::sin(rig.time * 0.23f) * 6.0f, std::sin(rig.time * 0.31f) * 4.0f, 0.0f);

                transform.position  = rig.basePosition + jolt + drift;
                transform.dirty     = true;
                camera.lookAtTarget = rig.baseLookAt + jolt * 0.5f;
                camera.dirty        = true;
            });
    }

    //----------------------------------------------------------------------------
    //! カメラを揺らします。
    //----------------------------------------------------------------------------
    void ShakeCamera(Tsukino::ECS::Registry& registry, float strength) {
        // 設定で揺れを切っていれば揺らさない（酔いやすい人向け）
        if(!Game::CurrentOptions().cameraShake) {
            return;
        }
        registry.View<CameraRigComponent>().each([strength](auto, CameraRigComponent& rig) { rig.shake = std::max(rig.shake, strength); });
    }

    //----------------------------------------------------------------------------
    //! 出来事の購読を始めます。
    //----------------------------------------------------------------------------
    MatchFeedbackSystem::MatchFeedbackSystem(Tsukino::ECS::Registry& registry, Tsukino::ECS::EventBus& events) {
        Tsukino::ECS::Registry* reg = &registry;
        auto sounds = [reg]() -> Game::SoundBank& { return Game::Sounds(Engine(*reg)); };
        auto session = [reg]() -> Game::MatchSession& { return Game::Session(*reg); };

        m_connections.push_back(events.Subscribe<Game::OpeningCardDealtEvent>([reg, sounds, session](const Game::OpeningCardDealtEvent& e) {
            // 山札から飛んで、降りたところで砂埃と金色の光
            Game::Sequencer& sequencer = session().sequencer;
            sequencer.Schedule(0.2f, [sounds] { sounds().Play("whoosh", 0.6f); });
            sequencer.Schedule(0.9f, [reg, sounds, session, card = e.card] {
                session().particles->Dust(reg->GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(card).position);
                Glow(*reg, card, kGoldGlow, 1.6f, 1.4f);
                sounds().Play("card_place");
            });
        }));
        m_connections.push_back(events.Subscribe<Game::CardDealtEvent>([sounds, session](const Game::CardDealtEvent& e) {
            session().sequencer.Schedule(e.delay, [sounds] { sounds().Play("deal", 0.55f); });
        }));
        m_connections.push_back(events.Subscribe<Game::TurnStartedEvent>([session](const Game::TurnStartedEvent& e) {
            Game::MatchSession& s = session();
            s.hud->SetTurn(e.player);
            if(e.human && s.config.mode == Rules::MatchMode::Versus) {
                s.hud->ShowBanner(L"あなたの番", false);
            }
        }));
        m_connections.push_back(events.Subscribe<Game::TiebreakStartedEvent>([sounds, session](const Game::TiebreakStartedEvent&) {
            session().hud->ShowBanner(L"延長戦！", true);
            sounds().Play("page");
        }));
        m_connections.push_back(events.Subscribe<Game::CpuThinkingStartedEvent>([sounds](const Game::CpuThinkingStartedEvent&) { sounds().Play("card_flip", 0.4f); }));
        m_connections.push_back(events.Subscribe<Game::CpuHoveredSlotEvent>([sounds](const Game::CpuHoveredSlotEvent&) { sounds().Play("tick", 0.3f); }));
        m_connections.push_back(events.Subscribe<Game::CardCommittedEvent>([sounds](const Game::CardCommittedEvent&) { sounds().Play("whoosh", 0.5f); }));
        m_connections.push_back(events.Subscribe<Game::CardFlippedEvent>([sounds](const Game::CardFlippedEvent&) { sounds().Play("card_flip"); }));
        m_connections.push_back(events.Subscribe<Game::CardLandedEvent>([reg, sounds, session](const Game::CardLandedEvent& e) {
            session().particles->Dust(e.at);
            ShakeCamera(*reg, 4.0f);
            sounds().Play("card_place");
        }));
        m_connections.push_back(events.Subscribe<Game::PlayJudgedEvent>([reg, sounds, session](const Game::PlayJudgedEvent& e) {
            Game::MatchSession& s = session();
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
        m_connections.push_back(events.Subscribe<Game::CorrectSlotShownEvent>([sounds](const Game::CorrectSlotShownEvent&) { sounds().Play("card_slide", 0.5f); }));
        m_connections.push_back(events.Subscribe<Game::CardDiscardedEvent>([sounds](const Game::CardDiscardedEvent&) { sounds().Play("whoosh", 0.6f); }));
        m_connections.push_back(events.Subscribe<Game::MatchEndedEvent>([reg, sounds, session](const Game::MatchEndedEvent& e) {
            Game::MatchSession& s = session();
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
        m_connections.push_back(events.Subscribe<Game::ResultRequestedEvent>([sounds, session](const Game::ResultRequestedEvent&) {
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
