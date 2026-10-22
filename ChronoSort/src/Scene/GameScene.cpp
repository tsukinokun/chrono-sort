//----------------------------------------------------------------------------
//! @file   GameScene.cpp
//! @brief  対局画面の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Scene/GameScene.hpp>

#include <ChronoSort/ECS/CardMotion.hpp>
#include <ChronoSort/ECS/System/CardGlowSystem.hpp>
#include <ChronoSort/ECS/System/CardMotionSystem.hpp>
#include <ChronoSort/ECS/System/ParticleSystem.hpp>
#include <ChronoSort/Game/SaveData.hpp>
#include <ChronoSort/Game/SoundBank.hpp>
#include <ChronoSort/Game/Sounds.hpp>
#include <ChronoSort/Rules/CpuPlayer.hpp>
#include <ChronoSort/Rules/Deck.hpp>
#include <ChronoSort/Scene/TitleScene.hpp>

#include <Tsukino/BuiltIn/ECS/Component/BloomComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/PointLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SkyAtmosphereComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Window.hpp>
#include <Tsukino/EngineIntegration/ECS/System/BloomSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/InteractionSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/LightSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/ModelSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SkyAtmosphereSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/UITweenSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/WorldAnchorSystem.hpp>
#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <entt/entt.hpp>

#include <algorithm>
#include <cmath>

namespace ChronoSort::Scene {

    namespace {
        namespace CameraUtility = Tsukino::BuiltIn::ECS::CameraUtility;
        using Tsukino::Input::KeyCode;

        const hlslpp::float3 kCameraPosition(0.0f, 500.0f, -520.0f);
        const hlslpp::float3 kCameraLookAt(0.0f, 0.0f, -60.0f);

        constexpr float kClickThreshold = 8.0f;      // これ以上動いたらクリックではなくドラッグ
        constexpr float kRowBandNear    = 130.0f;    // 場の列の手前、この距離までなら「列に置こうとしている」
        constexpr float kRowBandFar     = 160.0f;    // 場の列の奥、この距離まで
        constexpr float kEdgePanMargin  = 110.0f;    // 画面端のこの幅に入るとカメラが横へ動く
        constexpr float kEdgePanSpeed   = 420.0f;    // 画面端での横移動の速さ

        const hlslpp::float3 kGoldGlow(0.55f, 0.38f, 0.1f);
        const hlslpp::float3 kRedGlow(0.6f, 0.05f, 0.03f);
        const hlslpp::float3 kSelectGlow(0.22f, 0.16f, 0.05f);
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

        m_match.Start(m_config, Rules::LoadAllCards(), m_rng);

        m_factory   = std::make_unique<Board::CardFactory>(m_scene, *context->assetManager);
        m_board     = std::make_unique<Board::BoardView>(*m_factory);
        m_particles = std::make_unique<Effects::ParticleEmitter>(m_scene, *context->assetManager);
        m_hud       = std::make_unique<UI::GameHud>(m_scene, *context->assetManager, m_match);
        m_board->Build(m_match.Players().size());
        m_hud->Build();

        Game::Sounds(*context).PlayBgm("game");
        DealOpening();
    }

    //----------------------------------------------------------------------------
    //! シーンを更新します。
    //----------------------------------------------------------------------------
    void GameScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        auto* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        m_time += deltaTime;

        m_sequencer.Update(deltaTime);

        if(m_phase == Phase::HumanTurn) {
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

        // 対局中に Esc でタイトルへ（選択中なら先に選択を外す）
        if(context->inputSystem->IsKeyPressed(KeyCode::Escape)) {
            if(m_selected >= 0 || m_grabbed >= 0) {
                CancelSelection();
            } else {
                api.ChangeScene(std::make_unique<TitleScene>());
                return;
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

        m_board->SetDeckCount(m_match.DeckRemaining());
        m_board->UpdateLayout(deltaTime);
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
        // 画面 UI のマウス判定とトゥイーンは Transform の前に行う
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::InteractionSystem>(), -3);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::UITweenSystem>(), -2);
        // カードの動きと粒子も、Transform の計算より前に確定させる
        m_scene.AddSystem(std::make_shared<ECS::CardMotionSystem>(), -1);
        m_scene.AddSystem(std::make_shared<ECS::ParticleSystem>(), -1);
        m_scene.AddSystem(std::make_shared<ECS::CardGlowSystem>(), -1);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), 0);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::CameraSystem>(), 5);
        // カードに貼り付ける画面 UI（判定の印など）はカメラ行列の確定後
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::WorldAnchorSystem>(), 6);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::FontRendererSystem>(), 9);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SpriteRenderSystem>(), 10);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::ModelSystem>(), 10);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::LightSystem>(), 13);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SkyAtmosphereSystem>(), 14);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::BloomSystem>(), 15);
    }

    //----------------------------------------------------------------------------
    //! カメラ・ライト・後処理を配置します。
    //----------------------------------------------------------------------------
    void GameScene::CreateStage() {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();

        // 卓を手前上から見下ろす透視カメラ
        m_camera = m_scene.CreateEntity();
        registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(m_camera).position = kCameraPosition;
        {
            auto& cam        = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(m_camera);
            cam.fov          = 40.0f;
            cam.nearZ        = 10.0f;
            cam.farZ         = 5000.0f;
            cam.useLookAt    = true;
            cam.lookAtTarget = kCameraLookAt;
            cam.isPrimary    = true;
        }

        // 画面 UI 用のカメラ。基準解像度を決めておくと、ウィンドウの大きさを変えても UI が崩れない
        {
            Tsukino::ECS::Entity ui = m_scene.CreateEntity();
            registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(ui).position = hlslpp::float3(0.0f, 0.0f, -1.0f);
            auto& cam               = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(ui);
            cam.projectionType      = Tsukino::BuiltIn::ECS::CameraComponent::ProjectionType::Orthographic;
            cam.referenceResolution = hlslpp::float2(UI::kReferenceWidth, UI::kReferenceHeight);
            cam.isPrimary           = false;
        }

        // 斜め上からの主光源。卓にカードの影を落とす
        {
            auto& sun      = registry.AddComponent<Tsukino::BuiltIn::ECS::DirectionalLightComponent>(m_scene.CreateEntity());
            sun.direction  = hlslpp::normalize(hlslpp::float3(0.35f, -1.0f, 0.45f));
            sun.color      = hlslpp::float3(1.0f, 0.93f, 0.82f);
            sun.intensity  = 2.3f;
            sun.castShadow = true;
        }

        // 卓の中央を温かく照らす補助光（ランプの明かり）
        {
            Tsukino::ECS::Entity lamp = m_scene.CreateEntity();
            registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(lamp).position = hlslpp::float3(0.0f, 220.0f, 0.0f);
            auto& point     = registry.AddComponent<Tsukino::BuiltIn::ECS::PointLightComponent>(lamp);
            point.color     = hlslpp::float3(1.0f, 0.75f, 0.45f);
            point.intensity = 8.0f;
            point.range     = 700.0f;
        }

        // 環境光（IBL）の元。空そのものは卓に隠れて見えない
        registry.AddComponent<Tsukino::BuiltIn::ECS::SkyAtmosphereComponent>(m_scene.CreateEntity());

        // 正解の金色や火花を滲ませる
        {
            auto& bloom     = registry.AddComponent<Tsukino::BuiltIn::ECS::BloomComponent>(m_scene.CreateEntity());
            bloom.threshold = 1.0f;
            bloom.intensity = 0.5f;
        }
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
        m_board->UpdateLayout(0.0f);
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
                const auto&       hand = m_match.Players()[p].hand;
                Board::HandView&  view = m_board->Hand(p);
                if(view.cards.size() >= hand.size()) {
                    continue;
                }
                Tsukino::ECS::Entity entity = m_board->SpawnAtDeck(hand[view.cards.size()]);
                view.cards.push_back(entity);
                // 配る先の姿勢を先に決めてから飛ばす
                hlslpp::float3     position;
                hlslpp::quaternion rotation;
                float              scale = 1.0f;
                m_board->HandPose(p, view.cards.size() - 1, position, rotation, scale);
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
        Board::HandView&              hand     = m_board->Hand(0);

        int mouseX = 0;
        int mouseY = 0;
        input.GetMousePosition(&mouseX, &mouseY);
        const hlslpp::float2 mouse(static_cast<float>(mouseX), static_cast<float>(mouseY));
        const auto           ray = MouseRay();

        // ホイールで場の列を左右に見渡す
        m_panTarget -= input.GetWheelDelta() * 0.6f;

        // カーソルが重なっている手札（ドラッグ中・選択中は更新しない）
        if(!m_dragging && m_selected < 0) {
            hlslpp::float2 uv;
            const int      hovered = m_board->PickHandCard(0, ray, uv);
            if(hovered != hand.hovered && hovered >= 0) {
                Game::Sounds(*context).Play("card_slide", 0.25f);
            }
            hand.hovered   = hovered;
            hand.hoverTilt = hovered >= 0 ? hlslpp::float2((float(uv.x) - 0.5f) * 2.0f, (float(uv.y) - 0.5f) * 2.0f) : hlslpp::float2(0.0f, 0.0f);
        }

        // 手札を持っていないときは、指した場のカードを起こして年号を読めるようにする
        m_board->SetRowHovered((!m_dragging && m_selected < 0 && hand.hovered < 0) ? m_board->PickRowCard(ray) : -1);

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
            const Tsukino::ECS::Entity entity = hand.cards[static_cast<size_t>(m_grabbed)];
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

                    // 画面端に寄せたら列を見渡せるよう横へ動く
                    const float width = static_cast<float>(context->window->GetWidth());
                    if(float(mouse.x) < kEdgePanMargin) {
                        m_panTarget -= kEdgePanSpeed * deltaTime;
                    } else if(float(mouse.x) > width - kEdgePanMargin) {
                        m_panTarget += kEdgePanSpeed * deltaTime;
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
                const Tsukino::ECS::Entity entity = hand.cards[static_cast<size_t>(index)];
                m_board->SetHeld(entt::null);
                ECS::CardMotion::SetSharpness(registry, entity, 14.0f, 10.0f);
                m_board->UpdateLayout(0.0f);
                ECS::CardMotion::Fly(registry, entity, 0.35f, 30.0f);
                Game::Sounds(*context).Play("card_slide", 0.4f);
            } else {
                // クリック：選択して掲げる（ドラッグが苦手でも置けるように）
                m_selected  = index;
                hand.lifted = index;
                hand.hovered = -1;
                ECS::PulseGlow(registry, hand.cards[static_cast<size_t>(index)], kSelectGlow, 1.0f, 5.0f);
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
        Board::HandView&        hand     = m_board->Hand(0);
        if(m_selected >= 0 && static_cast<size_t>(m_selected) < hand.cards.size()) {
            ECS::StopGlow(registry, hand.cards[static_cast<size_t>(m_selected)]);
        }
        if(m_grabbed >= 0 && static_cast<size_t>(m_grabbed) < hand.cards.size()) {
            ECS::CardMotion::SetSharpness(registry, hand.cards[static_cast<size_t>(m_grabbed)], 14.0f, 10.0f);
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
        Board::HandView&     hand   = m_board->Hand(player);
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
        Board::HandView&        hand     = m_board->Hand(player);
        if(handIndex >= hand.cards.size()) {
            return;
        }

        const Tsukino::ECS::Entity entity = hand.cards[handIndex];
        const auto                 result = m_match.Play(handIndex, slot);
        if(!result) {
            return;
        }
        hand.cards.erase(hand.cards.begin() + static_cast<std::ptrdiff_t>(handIndex));
        hand.hovered = -1;
        hand.lifted  = -1;

        m_phase   = Phase::Resolving;
        m_preview = std::nullopt;
        m_board->SetRowHovered(-1);
        ECS::StopGlow(registry, entity);
        m_board->SetHeld(entity);
        m_board->SetGap(slot);
        m_board->SetMarker(false);

        // 置いた場所が画面の外なら、そこへカメラを寄せる
        m_panTarget = float(m_board->SlotPosition(slot).x);

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
            ECS::CardMotion::SetTarget(reg, entity, m_board->SlotPosition(slot) + up * 115.0f, Board::RevealedRotation(), 1.1f);
            Game::Sounds(*context).Play("card_flip");
        });

        // 3. 卓へ叩きつける
        m_sequencer.Schedule(0.98f, [this, entity, slot] {
            Tsukino::ECS::Registry& reg = m_scene.GetRegistry();
            ECS::CardMotion::SetSharpness(reg, entity, 34.0f, 16.0f);
            ECS::CardMotion::SetTarget(reg, entity, m_board->SlotPosition(slot), Board::RevealedRotation(), 1.0f);
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
                m_panTarget = float(m_board->SlotPosition(correctSlot).x);
                ECS::CardMotion::SetSharpness(r, entity, 8.0f, 8.0f);
                ECS::CardMotion::SetTarget(r, entity, m_board->SlotPosition(correctSlot) + up * 40.0f, Board::RevealedRotation(), 1.0f);
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
            m_hud->SetNewRecord(m_match.Score() > save.bestScore && m_match.Score() > 0);
            save.bestScore  = std::max(save.bestScore, m_match.Score());
            save.bestPlaced = std::max(save.bestPlaced, m_match.Players()[0].correctCount);
            save.bestStreak = std::max(save.bestStreak, m_match.BestStreak());
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
            m_hud->ShowResult();
            Game::Sounds(*context).Play("page");
            Game::Sounds(*context).PlayBgm("result");
        });
    }

    //----------------------------------------------------------------------------
    //! カメラの横移動と揺れを更新します。
    //----------------------------------------------------------------------------
    void GameScene::UpdateCamera(float deltaTime) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();

        // 列の端より外へは行かない（列が短ければ中央に留まる）
        const hlslpp::float2 extent = m_board->RowExtent();
        const float          limit  = std::max(0.0f, (float(extent.y) - float(extent.x)) * 0.5f - 300.0f);
        m_panTarget                 = std::clamp(m_panTarget, -limit, limit);
        m_pan += (m_panTarget - m_pan) * (1.0f - std::exp(-5.0f * deltaTime));

        // 揺れは減衰するランダムなずれ
        m_shake = std::max(0.0f, m_shake - deltaTime * 18.0f);
        const hlslpp::float3 jolt(std::sin(m_time * 71.0f) * m_shake, std::cos(m_time * 53.0f) * m_shake * 0.6f, 0.0f);
        // ゆっくり呼吸するように少しだけ漂わせ、止まった絵に見せない
        const hlslpp::float3 drift(std::sin(m_time * 0.23f) * 6.0f, std::sin(m_time * 0.31f) * 4.0f, 0.0f);

        auto& transform    = registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(m_camera);
        auto& camera       = registry.GetComponent<Tsukino::BuiltIn::ECS::CameraComponent>(m_camera);
        const hlslpp::float3 pan(m_pan, 0.0f, 0.0f);
        transform.position  = kCameraPosition + pan + jolt + drift;
        transform.dirty     = true;
        camera.lookAtTarget = kCameraLookAt + pan + jolt * 0.5f;
        camera.dirty        = true;
    }

    //----------------------------------------------------------------------------
    //! カメラを揺らします。
    //----------------------------------------------------------------------------
    void GameScene::ShakeCamera(float strength) {
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
