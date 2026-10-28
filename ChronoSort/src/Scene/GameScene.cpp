//----------------------------------------------------------------------------
//! @file   GameScene.cpp
//! @brief  対局画面の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Scene/GameScene.hpp>

#include <ChronoSort/ECS/CardMotion.hpp>
#include <ChronoSort/ECS/System/CardGlowSystem.hpp>
#include <ChronoSort/ECS/SystemSetup.hpp>
#include <ChronoSort/Game/FrameMonitor.hpp>
#include <ChronoSort/Game/Options.hpp>
#include <ChronoSort/Game/Prefabs.hpp>
#include <ChronoSort/Game/SaveData.hpp>
#include <ChronoSort/Game/SoundBank.hpp>
#include <ChronoSort/Game/Sounds.hpp>
#include <ChronoSort/Rules/CpuPlayer.hpp>
#include <ChronoSort/Rules/Deck.hpp>
#include <ChronoSort/Scene/TitleScene.hpp>

#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Window.hpp>
#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <entt/entt.hpp>

#include <algorithm>
#include <cmath>

namespace ChronoSort::Scene {

    namespace {
        namespace CameraUtility = Tsukino::BuiltIn::ECS::CameraUtility;
        using Tsukino::Input::KeyCode;


        constexpr float kClickThreshold = 8.0f;      // これ以上動いたらクリックではなくドラッグ
        constexpr float kRowBandNear    = 130.0f;    // 場の列の手前、この距離までなら「列に置こうとしている」
        constexpr float kRowBandFar     = 160.0f;    // 場の列の奥、この距離まで
        constexpr float kEdgePanMargin  = 0.08f;     // 画面端のこの割合の幅に入ると場の列が流れる
        constexpr float kEdgePanSpeed   = 420.0f;    // 画面端で列を流す速さ
        constexpr float kWheelScroll    = 60.0f;     // ホイール1段で列を流す量（ホイールの値は段数で来る）
        constexpr float kFastEffectsScale = 1.7f;    // 「演出を速く」のときの、自分の手番以外の時間の倍率

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
    }    // namespace

    //----------------------------------------------------------------------------
    //! 対局の設定を受け取ります。
    //----------------------------------------------------------------------------
    GameScene::GameScene(const Rules::MatchConfig& config)
        : m_config(config)
        , m_rng(std::random_device{}()) {
    }

    //----------------------------------------------------------------------------
    //! シーンを初期化します。
    //----------------------------------------------------------------------------
    void GameScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) {
        m_api         = &api;
        auto* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();

        RegisterSystems();
        CreateStage();

        m_match.Start(m_config, Rules::LoadCards(m_config.range), m_rng);

        m_factory   = std::make_unique<Board::CardFactory>(m_scene, *context->assetManager);
        m_board     = std::make_unique<Board::BoardView>(*m_factory);
        m_particles = std::make_unique<Effects::ParticleEmitter>(m_scene, *context->assetManager);
        m_hud       = std::make_unique<UI::GameHud>(m_scene, *context->assetManager, m_match);
        m_board->Build(m_match.Players().size());
        m_hud->Build();
        m_options = std::make_unique<UI::OptionsSheet>(m_scene, *context, 120);

        Game::Sounds(*context).PlayBgm("game");
        DealOpening();
    }

    //----------------------------------------------------------------------------
    //! シーンを更新します。
    //----------------------------------------------------------------------------
    void GameScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        auto* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();

        // 設定の紙（一時停止から開く）。閉じたフレームの Esc で一時停止まで閉じないよう、ここで先に受ける
        const bool optionsClosed = m_options->Update(deltaTime);
        const bool paused        = m_hud->IsPauseOpen() || m_options->IsOpen();

        // 「演出を速く」は自分で操作していない時間（配る・公開・判定・CPU の手番）だけを早回しする
        const bool  watching = m_phase != Phase::HumanTurn && m_phase != Phase::Finished;
        const float timeScale = (!paused && watching && Game::CurrentOptions().fastEffects) ? kFastEffectsScale : 1.0f;
        deltaTime *= timeScale;
        m_time += deltaTime;

        // 一時停止中は段取りを進めない（カードの動きや紙の揺れは止めない）
        if(!paused) {
            m_sequencer.Update(deltaTime);
        }

        // 人間の手番でなければ、カーソルではなく隙間（CPU の迷い・公開中）を注目点にする
        if(m_phase != Phase::HumanTurn) {
            m_board->SetFocusX(std::nullopt);
        }

        if(m_phase == Phase::HumanTurn && !paused) {
            UpdateHumanInput(deltaTime);
#ifdef _DEBUG
            // 確認用：F8 で人間の手番を CPU（やさしい）に代わりに打たせる
            if(m_phase == Phase::HumanTurn && context->inputSystem->IsKeyPressed(KeyCode::F8)) {
                CancelSelection();
                Rules::Player stand = m_match.Players()[0];
                stand.cpuLevel      = Rules::CpuLevel::Easy;
                const Rules::CpuMove move = Rules::ChooseCpuMove(stand, m_match.GetTimeline(), m_rng);
                CommitPlay(0, move.handIndex, move.slot);
            }
#endif
        }

        // Esc：選択中なら選択を外し、そうでなければ一時停止の紙を出し入れする（結果画面ではタイトルへ）
        if(!optionsClosed && !m_options->IsOpen() && context->inputSystem->IsKeyPressed(KeyCode::Escape)) {
            if(m_hud->IsPauseOpen()) {
                Game::Sounds(*context).Play("page", 0.7f);
                m_hud->ClosePause();
            } else if(m_selected >= 0 || m_grabbed >= 0) {
                CancelSelection();
            } else if(m_phase == Phase::Finished) {
                api.ChangeScene(std::make_unique<TitleScene>());
                return;
            } else {
                Game::Sounds(*context).Play("page", 0.8f);
                m_hud->OpenPause();
            }
        }
        if(!m_options->IsOpen()) {
            switch(m_hud->PollPauseChoice()) {
            case UI::PauseChoice::Resume:
                Game::Sounds(*context).Play("page", 0.7f);
                m_hud->ClosePause();
                break;
            case UI::PauseChoice::Options:
                Game::Sounds(*context).Play("page", 0.8f);
                m_options->Open();
                break;
            case UI::PauseChoice::Title:
                Game::Sounds(*context).Play("button");
                api.ChangeScene(std::make_unique<TitleScene>());
                return;
            default:
                break;
            }
        }

        if(m_phase == Phase::Finished) {
            switch(m_hud->PollResultChoice()) {
            case UI::ResultChoice::Retry:
                api.ChangeScene(std::make_unique<GameScene>(m_config));
                return;
            case UI::ResultChoice::Title:
                api.ChangeScene(std::make_unique<TitleScene>());
                return;
            default:
                break;
            }
        }

        // ホイールで場の列を左右に見渡す（CPU の手番や公開の演出中も見られるよう、手番を問わず受け付ける）
        m_board->ScrollBy(-context->inputSystem->GetWheelDelta() * kWheelScroll);

        // 人間の手番で操作していない時間（マウスを動かす・押す・ホイール・カードを持つ、で数え直す）
        {
            Tsukino::Input::InputSystem& input  = *context->inputSystem;
            int                          mouseX = 0;
            int                          mouseY = 0;
            input.GetMousePosition(&mouseX, &mouseY);
            const hlslpp::float2 mouse(static_cast<float>(mouseX), static_cast<float>(mouseY));
            const bool           active = float(hlslpp::length(mouse - m_lastMouse)) > 2.0f || input.IsKeyDown(KeyCode::LButton)
                                || input.IsKeyDown(KeyCode::RButton) || input.GetWheelDelta() != 0.0f || m_grabbed >= 0 || m_selected >= 0;
            m_lastMouse = mouse;
            m_idleTime  = (m_phase == Phase::HumanTurn && !active && !paused) ? m_idleTime + deltaTime : 0.0f;
            m_hud->SetIdleSeconds(m_idleTime);
        }

        // 卓の上の並べ直しは BoardLayoutSystem が m_scene.Update の中で行う
        m_board->SetDeckCount(m_match.DeckRemaining());
        UpdateCamera(deltaTime);
        m_hud->Update(deltaTime);

        m_scene.Update(deltaTime);
    }

    //----------------------------------------------------------------------------
    //! シーンの終了処理を行います。
    //----------------------------------------------------------------------------
    void GameScene::OnExit() {
        m_sequencer.Clear();
    }

    //----------------------------------------------------------------------------
    //! 描画と演出に使うシステムを登録します。
    //----------------------------------------------------------------------------
    void GameScene::RegisterSystems() {
        ECS::AddTableSystems(m_scene);
    }

    //----------------------------------------------------------------------------
    //! カメラ・ライト・後処理を配置します。
    //----------------------------------------------------------------------------
    void GameScene::CreateStage() {
        // カメラ（卓を手前上から見下ろす）・画面 UI のカメラ・主光源・ランプ・空（環境光の元）・Bloom は Prefab にある
        const Game::Prefabs::StageInstance stage = Game::Prefabs::SpawnGroup(m_scene.GetRegistry(), Game::Prefabs::kGameStage);
        m_camera                                 = stage.at("Camera");

        // 揺れと漂いは Prefab の姿勢を基準に足す
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        m_cameraBasePosition = registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(m_camera).position;
        m_cameraBaseLookAt   = registry.GetComponent<Tsukino::BuiltIn::ECS::CameraComponent>(m_camera).lookAtTarget;
    }

    //----------------------------------------------------------------------------
    //! 場の1枚目と手札を、山札から順に配ります。
    //----------------------------------------------------------------------------
    void GameScene::DealOpening() {
        m_phase                          = Phase::Dealing;
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        auto*                   context  = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();

        // 場の1枚目：山札から飛んで、空中で裏返って年号を見せる
        const Rules::CardData& first = m_match.GetTimeline().Cards().front();
        Tsukino::ECS::Entity   card  = m_board->SpawnAtDeck(first);
        m_board->InsertIntoRow(0, card);
        m_board->ApplyLayoutNow();
        ECS::CardMotion::Fly(registry, card, 0.7f, 120.0f, 0.2f);
        m_sequencer.Schedule(0.2f, [context] { Game::Sounds(*context).Play("whoosh", 0.6f); });
        m_sequencer.Schedule(0.9f, [this, card, context] {
            Tsukino::ECS::Registry& reg = m_scene.GetRegistry();
            m_particles->Dust(reg.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(card).position);
            ECS::Glow(reg, card, kGoldGlow, 1.6f, 1.4f);
            Game::Sounds(*context).Play("card_place");
        });

        const float lastArrival = SyncHands(1.1f);
        m_sequencer.Schedule(lastArrival + 0.3f, [this] { StartTurn(); });
    }

    //----------------------------------------------------------------------------
    //! 手札の見た目を Rules::Match の手札に合わせます。
    //----------------------------------------------------------------------------
    float GameScene::SyncHands(float delay) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        auto*                   context  = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        float                   last     = delay;
        float                   stagger  = delay;

        // 1人ずつではなく1枚ずつ順に配る（本物の配り方に近づける）
        bool dealt = true;
        while(dealt) {
            dealt = false;
            for(size_t p = 0; p < m_match.Players().size(); ++p) {
                const auto&  hand  = m_match.Players()[p].hand;
                const size_t shown = m_board->HandSize(p);
                if(shown >= hand.size()) {
                    continue;
                }
                Game::FrameMonitor::Mark("deal");
                Tsukino::ECS::Entity entity = m_board->SpawnAtDeck(hand[shown]);
                m_board->AddToHand(p, entity);
                // 配る先の姿勢を先に決めてから飛ばす
                hlslpp::float3     position;
                hlslpp::quaternion rotation;
                float              scale = 1.0f;
                m_board->HandPose(p, shown, position, rotation, scale);
                ECS::CardMotion::SetTarget(registry, entity, position, rotation, scale);
                ECS::CardMotion::Fly(registry, entity, 0.5f, 70.0f, stagger);
                m_sequencer.Schedule(stagger, [context] { Game::Sounds(*context).Play("deal", 0.55f); });
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
    void GameScene::StartTurn() {
        if(m_match.Outcome() != Rules::MatchOutcome::InProgress) {
            ShowResult();
            return;
        }
        const size_t player = m_match.CurrentPlayerIndex();
        m_hud->SetTurn(player);

        if(!m_match.Players()[player].isCpu) {
            m_phase = Phase::HumanTurn;
            if(m_config.mode == Rules::MatchMode::Versus) {
                m_hud->ShowBanner(L"あなたの番", false);
            }
            return;
        }
        m_phase = Phase::CpuTurn;
        RunCpuTurn();
    }

    //----------------------------------------------------------------------------
    //! 人間の手番の入力を処理します。
    //----------------------------------------------------------------------------
    void GameScene::UpdateHumanInput(float deltaTime) {
        Tsukino::ECS::Registry&       registry = m_scene.GetRegistry();
        auto*                         context  = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        Tsukino::Input::InputSystem&  input    = *context->inputSystem;
        Board::HandState&             hand     = m_board->Hand(0);

        int mouseX = 0;
        int mouseY = 0;
        input.GetMousePosition(&mouseX, &mouseY);
        const hlslpp::float2 mouse(static_cast<float>(mouseX), static_cast<float>(mouseY));
        const auto           ray = MouseRay();

        // カーソルが重なっている手札（ドラッグ中・選択中は更新しない）
        if(!m_dragging && m_selected < 0) {
            hlslpp::float2 uv;
            const int      hovered = m_board->PickHandCard(0, ray, uv);
            if(hovered != hand.hovered && hovered >= 0) {
                Game::FrameMonitor::Mark("hover hand card");
                Game::Sounds(*context).Play("card_slide", 0.25f);
            }
            hand.hovered   = hovered;
            hand.hoverTilt = hovered >= 0 ? hlslpp::float2((float(uv.x) - 0.5f) * 2.0f, (float(uv.y) - 0.5f) * 2.0f) : hlslpp::float2(0.0f, 0.0f);
        }

        // 手札を持っていないときは、指した場のカードを起こして年号を読めるようにする
        m_board->SetRowHovered((!m_dragging && m_selected < 0 && hand.hovered < 0) ? m_board->PickRowCard(ray) : -1);

        // 列の上のカーソル（持っているカード）の位置を注目点にして、周りのカードを読める大きさに広げる
        {
            std::optional<float> focus;
            hlslpp::float3       point    = m_dragPoint;
            float                distance = 0.0f;
            const bool           onTable  = m_dragging || CameraUtility::IntersectPlane(ray, hlslpp::float3(0.0f, 0.0f, 0.0f), hlslpp::float3(0.0f, 1.0f, 0.0f), distance);
            if(!m_dragging && onTable) {
                point = ray.GetPoint(distance);
            }
            if(onTable && float(point.z) >= Board::kRowZ - kRowBandNear && float(point.z) <= Board::kRowZ + kRowBandFar) {
                focus = float(point.x);
            }
            m_board->SetFocusX(focus);
        }

        // 置き先の候補：卓の上のカーソル位置が場の列の近くならスロットを決める
        auto previewFrom = [&](const hlslpp::float3& point) -> std::optional<size_t> {
            if(float(point.z) < Board::kRowZ - kRowBandNear || float(point.z) > Board::kRowZ + kRowBandFar) {
                return std::nullopt;
            }
            return m_board->SlotFromWorldX(point.x);
        };
        auto setPreview = [&](std::optional<size_t> slot) {
            if(slot != m_preview) {
                if(slot) {
                    Game::Sounds(*context).Play("tick", 0.35f);
                }
                m_preview = slot;
                m_board->SetGap(slot);
                m_board->SetMarker(slot.has_value());
            }
        };

        // 押した
        if(input.IsKeyPressed(KeyCode::LButton)) {
            if(m_selected >= 0) {
                // 選択中：列の上なら置く、それ以外なら選び直し
                if(m_preview) {
                    const size_t slot = *m_preview;
                    const int    card = m_selected;
                    m_selected        = -1;
                    hand.lifted       = -1;
                    CommitPlay(0, static_cast<size_t>(card), slot);
                    return;
                }
                const int previous = m_selected;
                CancelSelection();
                if(hand.hovered >= 0 && hand.hovered != previous) {
                    m_grabbed    = hand.hovered;
                    m_pressPoint = mouse;
                }
            } else if(hand.hovered >= 0) {
                m_grabbed    = hand.hovered;
                m_pressPoint = mouse;
                m_dragging   = false;
            }
        }

        // 押したまま動かした：ドラッグ開始・継続
        if(m_grabbed >= 0 && input.IsKeyDown(KeyCode::LButton)) {
            const Tsukino::ECS::Entity entity = m_board->HandCard(0, static_cast<size_t>(m_grabbed));
            if(!m_dragging && float(hlslpp::length(mouse - m_pressPoint)) > kClickThreshold) {
                m_dragging   = true;
                hand.hovered = -1;
                m_board->SetHeld(entity);
                ECS::CardMotion::SetSharpness(registry, entity, 28.0f, 16.0f);
                Game::Sounds(*context).Play("card_slide", 0.6f);
            }
            if(m_dragging) {
                float distance = 0.0f;
                if(CameraUtility::IntersectPlane(ray, hlslpp::float3(0.0f, Board::kDragHeight, 0.0f), hlslpp::float3(0.0f, 1.0f, 0.0f), distance)) {
                    const hlslpp::float3 point = ray.GetPoint(distance);
                    // 速度は少しなまらせてから傾きに使う（手ぶれで暴れないように）
                    const hlslpp::float3 velocity = (point - m_dragPoint) / std::max(deltaTime, 0.001f);
                    m_dragVelocity += (velocity - m_dragVelocity) * std::min(1.0f, deltaTime * 12.0f);
                    m_dragPoint = point;

                    const float              roll  = std::clamp(-float(m_dragVelocity.x) * 0.0011f, -0.4f, 0.4f);
                    const float              pitch = std::clamp(float(m_dragVelocity.z) * 0.0011f, -0.4f, 0.4f);
                    const hlslpp::quaternion lean  = hlslpp::mul(hlslpp::quaternion::rotation_x(pitch), hlslpp::quaternion::rotation_z(roll));
                    ECS::CardMotion::SetTarget(registry, entity, point, lean, 1.05f);

                    setPreview(previewFrom(hlslpp::float3(point.x, 0.0f, point.z)));

                    // 画面端（列を畳んで重ねているあたり）に寄せたら、列を流して続きを見せる
                    const float width  = static_cast<float>(context->window->GetWidth());
                    const float margin = width * kEdgePanMargin;
                    if(float(mouse.x) < margin) {
                        m_board->ScrollBy(-kEdgePanSpeed * deltaTime);
                    } else if(float(mouse.x) > width - margin) {
                        m_board->ScrollBy(kEdgePanSpeed * deltaTime);
                    }
                }
            }
        }

        // 離した
        if(m_grabbed >= 0 && input.IsKeyReleased(KeyCode::LButton)) {
            const int index = m_grabbed;
            m_grabbed       = -1;
            if(m_dragging) {
                m_dragging = false;
                if(m_preview) {
                    CommitPlay(0, static_cast<size_t>(index), *m_preview);
                    return;
                }
                // 置けない所で離した：手札へ放物線で戻す
                const Tsukino::ECS::Entity entity = m_board->HandCard(0, static_cast<size_t>(index));
                m_board->SetHeld(entt::null);
                ECS::CardMotion::SetSharpness(registry, entity, 14.0f, 10.0f);
                m_board->ApplyLayoutNow();
                ECS::CardMotion::Fly(registry, entity, 0.35f, 30.0f);
                Game::Sounds(*context).Play("card_slide", 0.4f);
            } else {
                // クリック：選択して掲げる（ドラッグが苦手でも置けるように）
                m_selected  = index;
                hand.lifted = index;
                hand.hovered = -1;
                ECS::PulseGlow(registry, m_board->HandCard(0, static_cast<size_t>(index)), kSelectGlow, 1.0f, 5.0f);
                Game::Sounds(*context).Play("card_flip", 0.4f);
            }
        }

        // 選択中は、卓の上のカーソル位置で置き先を予告する
        if(m_selected >= 0 && !m_dragging) {
            float distance = 0.0f;
            if(CameraUtility::IntersectPlane(ray, hlslpp::float3(0.0f, 0.0f, 0.0f), hlslpp::float3(0.0f, 1.0f, 0.0f), distance)) {
                setPreview(previewFrom(ray.GetPoint(distance)));
            }
            if(input.IsKeyPressed(KeyCode::RButton)) {
                CancelSelection();
            }
        } else if(!m_dragging && m_grabbed < 0) {
            setPreview(std::nullopt);
        }
    }

    //----------------------------------------------------------------------------
    //! 選択とドラッグの状態を解除し、手札へ戻します。
    //----------------------------------------------------------------------------
    void GameScene::CancelSelection() {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        Board::HandState&       hand     = m_board->Hand(0);
        const size_t            count    = m_board->HandSize(0);
        if(m_selected >= 0 && static_cast<size_t>(m_selected) < count) {
            ECS::StopGlow(registry, m_board->HandCard(0, static_cast<size_t>(m_selected)));
        }
        if(m_grabbed >= 0 && static_cast<size_t>(m_grabbed) < count) {
            ECS::CardMotion::SetSharpness(registry, m_board->HandCard(0, static_cast<size_t>(m_grabbed)), 14.0f, 10.0f);
        }
        m_selected  = -1;
        m_grabbed   = -1;
        m_dragging  = false;
        hand.lifted = -1;
        m_board->SetHeld(entt::null);
        m_preview = std::nullopt;
        m_board->SetGap(std::nullopt);
        m_board->SetMarker(false);
    }

    //----------------------------------------------------------------------------
    //! CPU の手番を進めます。
    //----------------------------------------------------------------------------
    void GameScene::RunCpuTurn() {
        const size_t         player = m_match.CurrentPlayerIndex();
        const Rules::CpuMove move   = Rules::ChooseCpuMove(m_match.Players()[player], m_match.GetTimeline(), m_rng);
        Board::HandState&    hand   = m_board->Hand(player);
        auto*                context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();

        // 自信が無いほど長く迷う
        std::uniform_real_distribution<float> unit(0.0f, 1.0f);
        const float think = 0.9f + (1.0f - move.confidence) * 1.6f + unit(m_rng) * 0.5f;

        hand.lifted = static_cast<int>(move.handIndex);
        Game::Sounds(*context).Play("card_flip", 0.4f);

        // 迷っている様子：途中で置き先の候補を行き来する
        const size_t slotCount = m_match.GetTimeline().SlotCount();
        const int    hops      = 1 + static_cast<int>((1.0f - move.confidence) * 3.0f);
        for(int i = 0; i < hops; ++i) {
            const float  at        = think * (0.35f + 0.5f * static_cast<float>(i) / static_cast<float>(hops));
            const int    wander    = std::uniform_int_distribution<int>(-1, 1)(m_rng);
            const size_t candidate = static_cast<size_t>(std::clamp(static_cast<int>(move.slot) + wander, 0, static_cast<int>(slotCount) - 1));
            m_sequencer.Schedule(at, [this, candidate, context] {
                m_board->SetGap(candidate);
                m_board->SetMarker(true);
                Game::Sounds(*context).Play("tick", 0.3f);
            });
        }

        m_sequencer.Schedule(think, [this, player, move] {
            m_board->Hand(player).lifted = -1;
            CommitPlay(player, move.handIndex, move.slot);
        });
    }

    //----------------------------------------------------------------------------
    //! 手札のカードを場に置き、公開と判定の演出を始めます。
    //----------------------------------------------------------------------------
    void GameScene::CommitPlay(size_t player, size_t handIndex, size_t slot) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        Board::HandState&       hand     = m_board->Hand(player);
        if(handIndex >= m_board->HandSize(player)) {
            return;
        }

        const auto result = m_match.Play(handIndex, slot);
        if(!result) {
            return;
        }
        const Tsukino::ECS::Entity entity = m_board->TakeFromHand(player, handIndex);
        hand.hovered = -1;
        hand.lifted  = -1;

        m_phase   = Phase::Resolving;
        m_preview = std::nullopt;
        m_board->SetRowHovered(-1);
        ECS::StopGlow(registry, entity);
        m_board->SetHeld(entity);
        m_board->SetGap(slot);
        m_board->SetMarker(false);

        // 置いた場所が端に畳まれているなら、そこまで列を流す
        m_board->ScrollToSlot(slot);

        Game::FrameMonitor::Mark("place card");
        RunReveal(player, entity, *result);
    }

    //----------------------------------------------------------------------------
    //! 置いたカードの公開・判定の演出を段取りします。
    //----------------------------------------------------------------------------
    void GameScene::RunReveal(size_t player, Tsukino::ECS::Entity entity, const Rules::PlayResult& result) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        auto*                   context  = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        const size_t            slot     = result.slot;
        const hlslpp::float3    up(0.0f, 1.0f, 0.0f);

        // 1. 隙間の上まで運ぶ（表のまま）
        ECS::CardMotion::SetTarget(registry, entity, m_board->SlotPosition(slot) + up * 60.0f, hlslpp::quaternion::identity(), 1.0f);
        ECS::CardMotion::Fly(registry, entity, 0.38f, 35.0f);
        Game::Sounds(*context).Play("whoosh", 0.5f);

        // 2. 持ち上げながら裏返す
        m_sequencer.Schedule(0.42f, [this, entity, slot, up, context] {
            Tsukino::ECS::Registry& reg = m_scene.GetRegistry();
            ECS::CardMotion::SetSharpness(reg, entity, 9.0f, 7.0f);
            ECS::CardMotion::SetTarget(reg, entity, m_board->SlotPosition(slot) + up * 115.0f, FacingRevealed(), m_board->SlotScale(slot) * 1.1f);
            Game::Sounds(*context).Play("card_flip");
        });

        // 3. 卓へ叩きつける
        m_sequencer.Schedule(0.98f, [this, entity, slot] {
            Tsukino::ECS::Registry& reg = m_scene.GetRegistry();
            ECS::CardMotion::SetSharpness(reg, entity, 34.0f, 16.0f);
            // 着地先は列に並んだときと同じ起こした姿勢（寝てから起き直す二段の動きを出さない）
            hlslpp::float3     position;
            hlslpp::quaternion rotation;
            float              scale = 1.0f;
            m_board->SlotRestPose(slot, position, rotation, scale);
            ECS::CardMotion::SetTarget(reg, entity, position, rotation, scale);
        });
        m_sequencer.Schedule(1.10f, [this, entity, slot, context] {
            Tsukino::ECS::Registry& reg = m_scene.GetRegistry();
            m_particles->Dust(m_board->SlotPosition(slot));
            ECS::CardMotion::Punch(reg, entity, 0.3f, 0.07f);
            ShakeCamera(4.0f);
            Game::Sounds(*context).Play("card_place");
        });

        // 4. 判定
        m_sequencer.Schedule(1.32f, [this, entity, result, player, up, context] {
            Tsukino::ECS::Registry& reg = m_scene.GetRegistry();
            ECS::CardMotion::SetSharpness(reg, entity, 14.0f, 10.0f);
            Game::FrameMonitor::Mark(result.correct ? "judge correct" : "judge wrong");
            m_hud->ShowStamp(entity, result.correct);
            Game::Sounds(*context).Play("stamp", 0.8f);
            if(result.correct) {
                ECS::Glow(reg, entity, kGoldGlow, 1.4f, 1.1f);
                m_particles->GoldBurst(m_board->SlotPosition(result.slot) + up * 8.0f);
                Game::Sounds(*context).Play("correct");
                m_board->InsertIntoRow(result.slot, entity);
                m_board->SetHeld(entt::null);
                if(m_config.mode == Rules::MatchMode::Solo) {
                    m_hud->ShowScorePopup(entity, result.scoreGained, result.closeCall, result.streak);
                } else if(result.closeCall) {
                    m_hud->ShowScorePopup(entity, 0, true, result.streak);
                }
                m_hud->OnPlayResolved(player, result);
                m_sequencer.Schedule(0.8f, [this] { FinishPlay(); });
                return;
            }

            // 不正解：赤く光って震え、正しい位置を見せてから捨て札へ
            ECS::Glow(reg, entity, kRedGlow, 1.0f, 0.6f);
            ECS::CardMotion::Shake(reg, entity, 0.5f, 7.0f);
            m_particles->InkSplash(m_board->SlotPosition(result.slot) + up * 6.0f);
            ShakeCamera(7.0f);
            Game::Sounds(*context).Play("wrong");
            m_hud->OnPlayResolved(player, result);

            const size_t correctSlot = result.correctSlots.empty() ? result.slot : result.correctSlots.front();
            m_sequencer.Schedule(0.75f, [this, entity, correctSlot, up, context] {
                Tsukino::ECS::Registry& r = m_scene.GetRegistry();
                m_board->SetGap(correctSlot);
                m_board->SetMarker(true, true);
                m_board->ScrollToSlot(correctSlot);
                ECS::CardMotion::SetSharpness(r, entity, 8.0f, 8.0f);
                ECS::CardMotion::SetTarget(r, entity, m_board->SlotPosition(correctSlot) + up * 40.0f, FacingRevealed(), m_board->SlotScale(correctSlot));
                Game::Sounds(*context).Play("card_slide", 0.5f);
            });
            m_sequencer.Schedule(1.9f, [this, entity, context] {
                Tsukino::ECS::Registry& r = m_scene.GetRegistry();
                m_board->SetGap(std::nullopt);
                m_board->SetMarker(false);
                m_board->SetHeld(entt::null);
                ECS::CardMotion::SetSharpness(r, entity, 14.0f, 10.0f);
                m_board->SendToDiscard(entity);
                Game::Sounds(*context).Play("whoosh", 0.6f);
            });
            m_sequencer.Schedule(2.4f, [this] { FinishPlay(); });
        });
    }

    //----------------------------------------------------------------------------
    //! 判定の演出のあと、手札の補充と手番の終わりを行います。
    //----------------------------------------------------------------------------
    void GameScene::FinishPlay() {
        // 補充（ソロ）・ペナルティ（対戦）で引いたカードを配る
        float wait = SyncHands(0.0f);

        m_sequencer.Schedule(wait + 0.15f, [this] {
            if(m_match.Outcome() != Rules::MatchOutcome::InProgress) {
                ShowResult();
                return;
            }
            m_match.AdvanceTurn();
            if(m_match.Outcome() != Rules::MatchOutcome::InProgress) {
                ShowResult();
                return;
            }

            // 延長戦が始まったら知らせて、追加の1枚を配る
            float delay = 0.0f;
            if(m_match.IsTiebreak() && !m_wasTiebreak) {
                m_wasTiebreak = true;
                m_hud->ShowBanner(L"延長戦！", true);
                Game::Sounds(*m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>()).Play("page");
                delay = SyncHands(0.8f) + 0.2f;
            }
            m_sequencer.Schedule(delay, [this] { StartTurn(); });
        });
    }

    //----------------------------------------------------------------------------
    //! 対局の終わりを演出し、結果を表示します。
    //----------------------------------------------------------------------------
    void GameScene::ShowResult() {
        if(m_phase == Phase::Finished) {
            return;
        }
        m_phase      = Phase::Finished;
        auto* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();

        const Rules::MatchOutcome outcome = m_match.Outcome();
        const bool                won     = outcome == Rules::MatchOutcome::SoloCleared ||
                                            (outcome == Rules::MatchOutcome::VersusWon && m_match.Winner() == 0);
        Game::Sounds(*context).Play(won ? "fanfare" : "lose");

        // 記録を更新して保存する
        Game::SaveData save = Game::LoadSaveData();
        if(m_config.mode == Rules::MatchMode::Solo) {
            // 記録は出題の範囲ごとに比べる
            Game::SoloRecord& record = save.Record(m_config.range);
            m_hud->SetNewRecord(m_match.Score() > record.bestScore && m_match.Score() > 0);
            record.bestScore  = std::max(record.bestScore, m_match.Score());
            record.bestPlaced = std::max(record.bestPlaced, m_match.Players()[0].correctCount);
            record.bestStreak = std::max(record.bestStreak, m_match.BestStreak());
        } else if(won) {
            ++save.versusWins;
        }
        Game::StoreSaveData(save);
        m_hud->ShowBanner(won ? L"見事！" : (outcome == Rules::MatchOutcome::SoloFailed ? L"インクが尽きた…" : L"惜しい…"), true);
        if(won) {
            // 場の列のカードを左から順に光らせる
            const auto& row = m_board->Row();
            for(size_t i = 0; i < row.size(); ++i) {
                const Tsukino::ECS::Entity entity = row[i];
                m_sequencer.Schedule(0.05f * static_cast<float>(i), [this, entity] {
                    ECS::Glow(m_scene.GetRegistry(), entity, kGoldGlow, 1.5f, 1.2f);
                    ECS::CardMotion::Punch(m_scene.GetRegistry(), entity, 0.3f, 0.06f);
                });
            }
        }
        m_sequencer.Schedule(1.4f, [this, context] {
            Game::FrameMonitor::Mark("show result");
            m_hud->ShowResult();
            Game::Sounds(*context).Play("page");
            Game::Sounds(*context).PlayBgm("result");
        });
    }

    //----------------------------------------------------------------------------
    //! カメラの揺れを更新します（列が長いときはカメラではなく列を流すので、横には動かさない）。
    //----------------------------------------------------------------------------
    void GameScene::UpdateCamera(float deltaTime) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();

        // 揺れは減衰するランダムなずれ
        m_shake = std::max(0.0f, m_shake - deltaTime * 18.0f);
        const hlslpp::float3 jolt(std::sin(m_time * 71.0f) * m_shake, std::cos(m_time * 53.0f) * m_shake * 0.6f, 0.0f);
        // ゆっくり呼吸するように少しだけ漂わせ、止まった絵に見せない
        const hlslpp::float3 drift(std::sin(m_time * 0.23f) * 6.0f, std::sin(m_time * 0.31f) * 4.0f, 0.0f);

        auto& transform    = registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(m_camera);
        auto& camera       = registry.GetComponent<Tsukino::BuiltIn::ECS::CameraComponent>(m_camera);
        transform.position  = m_cameraBasePosition + jolt + drift;
        transform.dirty     = true;
        camera.lookAtTarget = m_cameraBaseLookAt + jolt * 0.5f;
        camera.dirty        = true;
    }

    //----------------------------------------------------------------------------
    //! カメラを揺らします。
    //----------------------------------------------------------------------------
    void GameScene::ShakeCamera(float strength) {
        // 設定で揺れを切っていれば揺らさない（酔いやすい人向け）
        if(!Game::CurrentOptions().cameraShake) {
            return;
        }
        m_shake = std::max(m_shake, strength);
    }

    //----------------------------------------------------------------------------
    //! マウスのレイを返します。
    //----------------------------------------------------------------------------
    CameraUtility::Ray GameScene::MouseRay() {
        auto*       context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        const auto& camera  = m_scene.GetRegistry().GetComponent<Tsukino::BuiltIn::ECS::CameraComponent>(m_camera);
        int         x       = 0;
        int         y       = 0;
        context->inputSystem->GetMousePosition(&x, &y);
        const hlslpp::float2 size(static_cast<float>(context->window->GetWidth()), static_cast<float>(context->window->GetHeight()));
        return CameraUtility::ScreenPointToRay(camera, hlslpp::float2(static_cast<float>(x), static_cast<float>(y)), size);
    }

}    // namespace ChronoSort::Scene
