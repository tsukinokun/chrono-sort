//----------------------------------------------------------------------------
//! @file   HumanInputSystem.cpp
//! @brief  人間の入力を処理するシステムの実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/System/HumanInputSystem.hpp>

#include <ChronoSort/Board/BoardView.hpp>
#include <ChronoSort/ECS/CardMotion.hpp>
#include <ChronoSort/ECS/System/CardGlowSystem.hpp>
#include <ChronoSort/Game/FrameMonitor.hpp>
#include <ChronoSort/Game/GameServices.hpp>
#include <ChronoSort/Game/TuningSet.hpp>
#include <ChronoSort/Match/MatchActions.hpp>
#include <ChronoSort/UI/GameHud.hpp>

#include <Tsukino/BuiltIn/ECS/Camera/CameraUtility.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Window.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <algorithm>
#include <optional>

namespace ChronoSort::ECS {

    namespace {
        namespace CameraUtility = Tsukino::BuiltIn::ECS::CameraUtility;
        using Tsukino::Input::KeyCode;



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
        void UpdateHandInput(Tsukino::ECS::Registry& registry, Match::MatchSession& session, float deltaTime) {
            auto&                        context = Engine(registry);
            Tsukino::Input::InputSystem& keys    = *context.inputSystem;
            Board::BoardView&            board   = *session.board;
            Board::HandState&            hand    = board.Hand(0);
            Match::HumanInput&            input   = session.input;
            Game::SoundBank&             sounds  = Game::Services().Sounds();
            const Match::InputTuning&    tuning  = Game::Tuning().Input();
            const float                  rowZ    = Board::Tuning().rowZ;

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
                    sounds.Play("hand.hover");
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
                if(onTable && float(point.z) >= rowZ - tuning.rowBandNear && float(point.z) <= rowZ + tuning.rowBandFar) {
                    focus = float(point.x);
                }
                board.SetFocusX(focus);
            }

            // 置き先の候補：卓の上のカーソル位置が場の列の近くならスロットを決める
            auto previewFrom = [&](const hlslpp::float3& point) -> std::optional<size_t> {
                if(float(point.z) < rowZ - tuning.rowBandNear || float(point.z) > rowZ + tuning.rowBandFar) {
                    return std::nullopt;
                }
                return board.SlotFromWorldX(point.x);
            };
            auto setPreview = [&](std::optional<size_t> slot) {
                if(slot != input.preview) {
                    if(slot) {
                        sounds.Play("slot.preview");
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
                        Match::CommitPlay(registry, 0, static_cast<size_t>(card), slot);
                        return;
                    }
                    const int previous = input.selected;
                    Match::CancelSelection(registry);
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
                if(!input.dragging && float(hlslpp::length(mouse - input.pressPoint)) > tuning.clickThreshold) {
                    input.dragging = true;
                    hand.hovered   = -1;
                    board.SetHeld(entity);
                    CardMotion::SetSharpness(registry, entity, tuning.dragSharpness);
                    sounds.Play("hand.drag");
                }
                if(input.dragging) {
                    float distance = 0.0f;
                    if(CameraUtility::IntersectPlane(ray, hlslpp::float3(0.0f, Board::Tuning().dragHeight, 0.0f), hlslpp::float3(0.0f, 1.0f, 0.0f), distance)) {
                        const hlslpp::float3 point = ray.GetPoint(distance);
                        // 速度は少しなまらせてから傾きに使う（手ぶれで暴れないように）
                        const hlslpp::float3 velocity = (point - input.dragPoint) / std::max(deltaTime, 0.001f);
                        input.dragVelocity += (velocity - input.dragVelocity) * std::min(1.0f, deltaTime * tuning.dragVelocitySmoothing);
                        input.dragPoint = point;

                        const float              roll  = std::clamp(-float(input.dragVelocity.x) * tuning.dragLeanPerSpeed, -tuning.dragLeanMax, tuning.dragLeanMax);
                        const float              pitch = std::clamp(float(input.dragVelocity.z) * tuning.dragLeanPerSpeed, -tuning.dragLeanMax, tuning.dragLeanMax);
                        const hlslpp::quaternion lean  = hlslpp::mul(hlslpp::quaternion::rotation_x(pitch), hlslpp::quaternion::rotation_z(roll));
                        CardMotion::SetTarget(registry, entity, point, lean, tuning.dragScale);

                        setPreview(previewFrom(hlslpp::float3(point.x, 0.0f, point.z)));

                        // 画面端（列を畳んで重ねているあたり）に寄せたら、列を流して続きを見せる
                        const float width  = static_cast<float>(context.window->GetWidth());
                        const float margin = width * tuning.edgePanMargin;
                        if(float(mouse.x) < margin) {
                            board.ScrollBy(-tuning.edgePanSpeed * deltaTime);
                        } else if(float(mouse.x) > width - margin) {
                            board.ScrollBy(tuning.edgePanSpeed * deltaTime);
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
                        Match::CommitPlay(registry, 0, static_cast<size_t>(index), *input.preview);
                        return;
                    }
                    // 置けない所で離した：手札へ放物線で戻す
                    const Tsukino::ECS::Entity entity = board.HandCard(0, static_cast<size_t>(index));
                    board.SetHeld(entt::null);
                    CardMotion::SetSharpness(registry, entity, tuning.returnSharpness);
                    board.ApplyLayoutNow();
                    CardMotion::Fly(registry, entity, tuning.returnFlight);
                    sounds.Play("hand.return");
                } else {
                    // クリック：選択して掲げる（ドラッグが苦手でも置けるように）
                    input.selected = index;
                    hand.lifted    = index;
                    hand.hovered   = -1;
                    PulseGlow(registry, board.HandCard(0, static_cast<size_t>(index)), tuning.selectGlow);
                    sounds.Play("hand.select");
                }
            }

            // 選択中は、卓の上のカーソル位置で置き先を予告する
            if(input.selected >= 0 && !input.dragging) {
                float distance = 0.0f;
                if(CameraUtility::IntersectPlane(ray, hlslpp::float3(0.0f, 0.0f, 0.0f), hlslpp::float3(0.0f, 1.0f, 0.0f), distance)) {
                    setPreview(previewFrom(ray.GetPoint(distance)));
                }
                if(keys.IsKeyPressed(KeyCode::RButton)) {
                    Match::CancelSelection(registry);
                }
            } else if(!input.dragging && input.grabbed < 0) {
                setPreview(std::nullopt);
            }
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! 人間の入力を処理します。
    //----------------------------------------------------------------------------
    void HumanInputSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        if(!Match::HasSession(registry)) {
            return;
        }
        Match::MatchSession&          session = Match::Session(registry);
        Tsukino::Input::InputSystem& keys    = *Engine(registry).inputSystem;

        // 人間の手番でなければ、カーソルではなく隙間（CPU の迷い・公開中）を注目点にする
        if(session.phase != Match::MatchPhase::HumanTurn) {
            session.board->SetFocusX(std::nullopt);
        }

        if(session.phase == Match::MatchPhase::HumanTurn && !session.paused) {
            UpdateHandInput(registry, session, deltaTime);
#ifdef _DEBUG
            // 確認用：F8 で人間の手番を CPU（やさしい）に代わりに打たせる
            if(session.phase == Match::MatchPhase::HumanTurn && keys.IsKeyPressed(KeyCode::F8)) {
                Match::CancelSelection(registry);
                Rules::Player stand = session.match.Players()[0];
                stand.cpuLevel      = Rules::CpuLevel::Easy;
                const Rules::CpuMove move = Rules::ChooseCpuMove(stand, session.match.GetTimeline(), session.rng, session.config.rules);
                Match::CommitPlay(registry, 0, move.handIndex, move.slot);
            }
#endif
        }

        // ホイールで場の列を左右に見渡す（CPU の手番や公開の演出中も見られるよう、手番を問わず受け付ける）
        const Match::InputTuning& tuning = Game::Tuning().Input();
        session.board->ScrollBy(-keys.GetWheelDelta() * tuning.wheelScroll);

        // 人間の手番で操作していない時間（マウスを動かす・押す・ホイール・カードを持つ、で数え直す）
        Match::HumanInput& input  = session.input;
        int               mouseX = 0;
        int               mouseY = 0;
        keys.GetMousePosition(&mouseX, &mouseY);
        const hlslpp::float2 mouse(static_cast<float>(mouseX), static_cast<float>(mouseY));
        const bool           active = float(hlslpp::length(mouse - input.lastMouse)) > tuning.idleMouseThreshold || keys.IsKeyDown(KeyCode::LButton) ||
                                      keys.IsKeyDown(KeyCode::RButton) || keys.GetWheelDelta() != 0.0f || input.grabbed >= 0 || input.selected >= 0;
        input.lastMouse = mouse;
        input.idleTime  = (session.phase == Match::MatchPhase::HumanTurn && !active && !session.paused) ? input.idleTime + deltaTime : 0.0f;
        session.hud->SetIdleSeconds(input.idleTime);
    }

}    // namespace ChronoSort::ECS
