//----------------------------------------------------------------------------
//! @file   GameScene.cpp
//! @brief  対局画面の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Scene/GameScene.hpp>

#include <ChronoSort/ECS/Component/CameraRigComponent.hpp>
#include <ChronoSort/ECS/SystemSetup.hpp>
#include <ChronoSort/Game/MatchActions.hpp>
#include <ChronoSort/Game/GameServices.hpp>
#include <ChronoSort/Game/Prefabs.hpp>
#include <ChronoSort/Rules/Deck.hpp>
#include <ChronoSort/Scene/TitleScene.hpp>

#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

namespace ChronoSort::Scene {

    namespace {
        using Tsukino::Input::KeyCode;

        constexpr float kFastEffectsScale = 1.7f;    // 「演出を速く」のときの、自分の手番以外の時間の倍率
    }    // namespace

    //----------------------------------------------------------------------------
    //! 対局の設定を受け取ります。
    //----------------------------------------------------------------------------
    GameScene::GameScene(const Rules::MatchConfig& config)
        : m_config(config) {
    }

    //----------------------------------------------------------------------------
    //! シーンを初期化します。
    //----------------------------------------------------------------------------
    void GameScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI&) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        auto*                   context  = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();

        // 対局の状態（規則の進行は Rules::Match。画面 UI がこれを読むので先に始める）
        m_session         = std::make_unique<Game::MatchSession>();
        m_session->config = m_config;
        m_session->rng.seed(std::random_device{}());
        m_session->match.Start(m_config, Rules::LoadCards(m_config.range), m_session->rng);

        ECS::AddTableSystems(m_scene);
        CreateStage();

        m_factory   = std::make_unique<Board::CardFactory>(m_scene, *context->assetManager);
        m_board     = std::make_unique<Board::BoardView>(*m_factory);
        m_particles = std::make_unique<Effects::ParticleEmitter>(m_scene, *context->assetManager);
        m_hud       = std::make_unique<UI::GameHud>(m_scene, *context->assetManager, m_session->match);
        m_board->Build(m_session->match.Players().size());
        m_hud->Build();
        m_options = std::make_unique<UI::OptionsSheet>(m_scene, *context, 120);

        // 対局を進めるシステムが使う窓口を渡し、システムを並べる
        m_session->board     = m_board.get();
        m_session->hud       = m_hud.get();
        m_session->particles = m_particles.get();
        m_session->events    = &m_scene.GetEventBus();
        registry.SetContext<Game::MatchSession*>(m_session.get());
        ECS::AddMatchSystems(m_scene);

        Game::Services().Sounds().PlayBgm("game");
        Game::DealOpening(registry);
    }

    //----------------------------------------------------------------------------
    //! シーンを更新します。
    //----------------------------------------------------------------------------
    void GameScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        if(UpdateMenus(api, deltaTime)) {
            return;
        }

        // 結果画面からの切り替え
        if(m_session->phase == Game::MatchPhase::Finished) {
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

        // 「演出を速く」は自分で操作していない時間（配る・公開・判定・CPU の手番）だけを早回しする
        const bool  watching  = m_session->phase != Game::MatchPhase::HumanTurn && m_session->phase != Game::MatchPhase::Finished;
        const float timeScale = (!m_session->paused && watching && Game::Services().Settings().fastEffects) ? kFastEffectsScale : 1.0f;
        deltaTime *= timeScale;

        m_board->SetDeckCount(m_session->match.DeckRemaining());
        m_hud->Update(deltaTime);

        // 対局の進行・卓の並べ方・カードの動きは、すべてシステムが m_scene.Update の中で行う
        m_scene.Update(deltaTime);
    }

    //----------------------------------------------------------------------------
    //! 一時停止の紙と設定の紙を扱います。
    //----------------------------------------------------------------------------
    bool GameScene::UpdateMenus(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        auto*                   context  = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        Game::SoundBank&        sounds   = Game::Services().Sounds();

        // 設定の紙（一時停止から開く）。閉じたフレームの Esc で一時停止まで閉じないよう、先に受ける
        const bool optionsClosed = m_options->Update(deltaTime);

        // Esc：選択中なら選択を外し、そうでなければ一時停止の紙を出し入れする（結果画面ではタイトルへ）
        if(!optionsClosed && !m_options->IsOpen() && context->inputSystem->IsKeyPressed(KeyCode::Escape)) {
            const Game::HumanInput& input = m_session->input;
            if(m_hud->IsPauseOpen()) {
                sounds.Play("page", 0.7f);
                m_hud->ClosePause();
            } else if(input.selected >= 0 || input.grabbed >= 0) {
                Game::CancelSelection(registry);
            } else if(m_session->phase == Game::MatchPhase::Finished) {
                api.ChangeScene(std::make_unique<TitleScene>());
                return true;
            } else {
                sounds.Play("page", 0.8f);
                m_hud->OpenPause();
            }
        }
        if(!m_options->IsOpen()) {
            switch(m_hud->PollPauseChoice()) {
            case UI::PauseChoice::Resume:
                sounds.Play("page", 0.7f);
                m_hud->ClosePause();
                break;
            case UI::PauseChoice::Options:
                sounds.Play("page", 0.8f);
                m_options->Open();
                break;
            case UI::PauseChoice::Title:
                sounds.Play("button");
                api.ChangeScene(std::make_unique<TitleScene>());
                return true;
            default:
                break;
            }
        }

        // 一時停止中は、対局の進行のシステムが段取り・入力・演出を止める（カードの動きや紙の揺れは止めない）
        m_session->paused = m_hud->IsPauseOpen() || m_options->IsOpen();
        return false;
    }

    //----------------------------------------------------------------------------
    //! シーンの終了処理を行います。
    //----------------------------------------------------------------------------
    void GameScene::OnExit() {
        m_session->sequencer.Clear();
    }

    //----------------------------------------------------------------------------
    //! カメラ・ライト・後処理を配置します。
    //----------------------------------------------------------------------------
    void GameScene::CreateStage() {
        // カメラ（卓を手前上から見下ろす）・画面 UI のカメラ・主光源・ランプ・空（環境光の元）・Bloom は Prefab にある
        Tsukino::ECS::Registry&            registry = m_scene.GetRegistry();
        const Game::Prefabs::StageInstance stage    = Game::Prefabs::SpawnGroup(registry, Game::Prefabs::kGameStage);
        m_session->camera                           = stage.at("Camera");

        // 揺れと漂いは Prefab の姿勢を基準に足す
        auto& rig        = registry.AddComponent<ECS::CameraRigComponent>(m_session->camera);
        rig.basePosition = registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(m_session->camera).position;
        rig.baseLookAt   = registry.GetComponent<Tsukino::BuiltIn::ECS::CameraComponent>(m_session->camera).lookAtTarget;
    }

}    // namespace ChronoSort::Scene
