//----------------------------------------------------------------------------
//! @file   GameScene.hpp
//! @brief  対局画面（ソロ・CPU 対戦）
//! @detail ここは組み立てと画面の切り替えだけを行います。
//!         - 組み立て：ステージの Prefab、卓（BoardView）・画面 UI（GameHud）・粒子を作り、対局の状態（MatchSession）を
//!           レジストリに置いて、対局を進めるシステム（ECS/System/MatchSystems）を並べる
//!         - 毎フレーム：一時停止の紙・設定の紙と、演出の速さ（時間の倍率）を扱い、結果画面からの切り替えを行う
//!         対局の進行（手番・CPU・人間の入力・公開と判定の演出）はシステムが、音や粒子は出来事を受けた
//!         MatchFeedbackSystem が行います。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Board/BoardView.hpp>
#include <ChronoSort/Board/CardFactory.hpp>
#include <ChronoSort/Effects/ParticleEmitter.hpp>
#include <ChronoSort/Game/MatchSession.hpp>
#include <ChronoSort/Rules/Match.hpp>
#include <ChronoSort/UI/GameHud.hpp>
#include <ChronoSort/UI/OptionsSheet.hpp>

#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>

#include <memory>

namespace ChronoSort::Scene {

    //! 対局画面です。
    class GameScene : public Tsukino::EngineIntegration::GameSceneBase {
    public:

        //! 対局の設定を受け取ります。
        //! @param  [in] config 対局の設定。
        explicit GameScene(const Rules::MatchConfig& config);
        ~GameScene() override = default;

        //! シーンを更新します。
        //! @param  [in] api       エンジン API。
        //! @param  [in] deltaTime 経過時間（秒）。
        void OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) override;

        //! シーンの終了処理を行います。
        void OnExit() override;

    private:

        //! シーンを初期化します。
        //! @param  [in] api エンジン API。
        void OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) override;

        //! カメラ・ライト・後処理を配置します（ステージの Prefab）。
        void CreateStage();

        //! 一時停止の紙と設定の紙を扱います。
        //! @param  [in] api       エンジン API（タイトルへ戻るとき）。
        //! @param  [in] deltaTime 経過時間（秒）。
        //! @return シーンを切り替えたら true。
        bool UpdateMenus(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime);

        Rules::MatchConfig                        m_config;      // 対局の設定（もう一度のときに使う）
        std::unique_ptr<Game::MatchSession>       m_session;     // 対局の状態（レジストリにはこれを指すポインタを置く）
        std::unique_ptr<Board::CardFactory>       m_factory;     // カードの生成
        std::unique_ptr<Board::BoardView>         m_board;       // 卓の置き場所の窓口
        std::unique_ptr<Effects::ParticleEmitter> m_particles;   // 粒子の演出
        std::unique_ptr<UI::GameHud>              m_hud;         // 画面 UI
        std::unique_ptr<UI::OptionsSheet>         m_options;     // 設定の紙（一時停止から開く）
    };

}    // namespace ChronoSort::Scene
