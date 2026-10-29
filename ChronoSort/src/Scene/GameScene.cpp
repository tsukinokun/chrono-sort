//----------------------------------------------------------------------------
//! @file   GameScene.cpp
//! @brief  対局画面の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Scene/GameScene.hpp>

#include <ChronoSort/Board/BoardView.hpp>
#include <ChronoSort/Board/CardFactory.hpp>
#include <ChronoSort/ECS/Component/CameraRigComponent.hpp>
#include <ChronoSort/Effects/ParticleEmitter.hpp>
#include <ChronoSort/ECS/SystemSetup.hpp>
#include <ChronoSort/Match/MatchActions.hpp>
#include <ChronoSort/Game/TuningSet.hpp>
#include <ChronoSort/Game/GameServices.hpp>
#include <ChronoSort/Game/Prefabs.hpp>
#include <ChronoSort/Rules/Deck.hpp>
#include <ChronoSort/Scene/SceneRouter.hpp>

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
        // ルールの調整値は対局を始めるときの Rules.json の値（対局の途中で読み直しても変えない）
        m_config.rules = Game::Tuning().Rules();
    }

    //----------------------------------------------------------------------------
    //! シーンを初期化します。
    //----------------------------------------------------------------------------
    void GameScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI&) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        auto*                   context  = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();

        // 対局の状態（規則の進行は Rules::Match。画面 UI がこれを読むので先に始める）
        Match::MatchSession& session = registry.SetContext<Match::MatchSession>();
        session.config               = m_config;
        session.rng.seed(std::random_device{}());
        session.match.Start(m_config, Rules::LoadCards(m_config.range), session.rng);

        ECS::AddTableSystems(m_scene);
        CreateStage();

        // 卓と粒子の窓口はレジストリに置き、システムからは Board::View・Effects::Particles で受け取る
        Board::CardFactory& factory = registry.SetContext<Board::CardFactory>(m_scene, *context->assetManager);
        registry.SetContext<Board::BoardView>(factory).Build(session.match.Players().size());
        registry.SetContext<Effects::ParticleEmitter>(m_scene, *context->assetManager);

        // 画面 UI は対局の出来事を受けて動く
        m_desk    = std::make_unique<UI::DeskHud>(m_scene, *context->assetManager, session.match);
        m_popups  = std::make_unique<UI::PlayPopups>(m_scene, *context->assetManager, session.match);
        m_result  = std::make_unique<UI::ResultSheet>(m_scene, *context->assetManager, session.match);
        m_album   = std::make_unique<UI::CardAlbum>(m_scene, *context->assetManager, session.match);
        m_pause   = std::make_unique<UI::PauseSheet>(m_scene, *context->assetManager);
        m_options = std::make_unique<UI::OptionsSheet>(m_scene, *context, 120);
        m_desk->Build();

        ECS::AddMatchSystems(m_scene);

        Game::Services().Sounds().PlayBgm("game");
        Match::DealOpening(registry);
    }

    //----------------------------------------------------------------------------
    //! シーンを更新します。
    //----------------------------------------------------------------------------
    void GameScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        if(UpdateMenus(api, deltaTime)) {
            return;
        }

        Match::MatchSession& session = Match::Session(m_scene.GetRegistry());

        // 結果画面からの切り替え（出たカードの紙が出ている間は、そちらだけを受ける）
        if(session.phase == Match::MatchPhase::Finished && m_album->IsOpen()) {
            auto* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
            m_album->Update(*context->inputSystem);
        } else if(session.phase == Match::MatchPhase::Finished) {
            switch(m_result->Poll()) {
            case UI::ResultChoice::Cards:
                Game::Services().Sounds().Play("ui.open");
                m_album->Open();
                break;
            case UI::ResultChoice::Retry:
                Router::ToMatch(api, m_config);
                return;
            case UI::ResultChoice::Title:
                Router::ToTitle(api);
                return;
            default:
                break;
            }
        }

        // 「演出を速く」は自分で操作していない時間（配る・公開・判定・CPU の手番）だけを早回しする
        const bool  watching  = session.phase != Match::MatchPhase::HumanTurn && session.phase != Match::MatchPhase::Finished;
        const float timeScale = (!session.paused && watching && Game::Services().Settings().fastEffects) ? kFastEffectsScale : 1.0f;
        deltaTime *= timeScale;

        // 対局の進行・卓の並べ方・カードの動き・画面 UI の書き換えは、すべてシステムと出来事が m_scene.Update の中で行う
        m_scene.Update(deltaTime);
    }

    //----------------------------------------------------------------------------
    //! 一時停止の紙と設定の紙を扱います。
    //----------------------------------------------------------------------------
    bool GameScene::UpdateMenus(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        auto*                   context  = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        Game::SoundBank&        sounds   = Game::Services().Sounds();
        Match::MatchSession&    session  = Match::Session(registry);

        // 設定の紙（一時停止から開く）。閉じたフレームの Esc で一時停止まで閉じないよう、先に受ける
        const bool optionsClosed = m_options->Update(deltaTime);

        // Esc：選択中なら選択を外し、そうでなければ一時停止の紙を出し入れする（結果画面ではタイトルへ。出たカードの紙が出ていれば1段戻る）
        if(!optionsClosed && !m_options->IsOpen() && context->inputSystem->IsKeyPressed(KeyCode::Escape)) {
            const Match::HumanInput& input = session.input;
            if(m_pause->IsOpen()) {
                sounds.Play("ui.close");
                m_pause->Close();
            } else if(m_album->IsOpen()) {
                sounds.Play("ui.close");
                m_album->Back();
            } else if(input.selected >= 0 || input.grabbed >= 0) {
                Match::CancelSelection(registry);
            } else if(session.phase == Match::MatchPhase::Finished) {
                Router::ToTitle(api);
                return true;
            } else {
                sounds.Play("ui.open");
                m_pause->Open();
            }
        }
        if(!m_options->IsOpen()) {
            switch(m_pause->Poll()) {
            case UI::PauseChoice::Resume:
                sounds.Play("ui.close");
                m_pause->Close();
                break;
            case UI::PauseChoice::Options:
                sounds.Play("ui.open");
                m_options->Open();
                break;
            case UI::PauseChoice::Title:
                sounds.Play("ui.button");
                Router::ToTitle(api);
                return true;
            default:
                break;
            }
        }

        // 一時停止中は、対局の進行のシステムが段取り・入力・演出を止める（カードの動きや紙の揺れは止めない）
        session.paused = m_pause->IsOpen() || m_options->IsOpen();
        return false;
    }

    //----------------------------------------------------------------------------
    //! シーンの終了処理を行います。
    //----------------------------------------------------------------------------
    void GameScene::OnExit() {
        // 予約した見た目の時間差は画面 UI やカードを指しているので、片付ける前に捨てる
        Match::Session(m_scene.GetRegistry()).sequencer.Clear();
    }

    //----------------------------------------------------------------------------
    //! カメラ・ライト・後処理を配置します。
    //----------------------------------------------------------------------------
    void GameScene::CreateStage() {
        // カメラ（卓を手前上から見下ろす）・画面 UI のカメラ・主光源・ランプ・空（環境光の元）・Bloom は Prefab にある
        Tsukino::ECS::Registry&            registry = m_scene.GetRegistry();
        const Game::Prefabs::StageInstance stage  = Game::Prefabs::SpawnGroup(registry, Game::Prefabs::kGameStage);
        const Tsukino::ECS::Entity         camera = stage.at("Camera");
        Match::Session(registry).camera           = camera;

        // 揺れと漂いは Prefab の姿勢を基準に足す
        auto& rig        = registry.AddComponent<ECS::CameraRigComponent>(camera);
        rig.basePosition = registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(camera).position;
        rig.baseLookAt   = registry.GetComponent<Tsukino::BuiltIn::ECS::CameraComponent>(camera).lookAtTarget;
    }

}    // namespace ChronoSort::Scene
