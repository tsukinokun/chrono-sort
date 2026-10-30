//----------------------------------------------------------------------------
//! @file   GameScene.hpp
//! @brief  対局画面（ソロ・CPU 対戦）
//! @detail ここは組み立てと画面の切り替えだけを行います。
//!         - 組み立て：ステージの Prefab と、対局の状態（MatchSession）・卓（BoardView）・粒子をレジストリのコンテキストに置き、
//!           画面 UI（DeskHud・PlayPopups・ResultSheet・PauseSheet）を作って、対局を進めるシステムを並べる
//!         - 毎フレーム：一時停止の紙・設定の紙と、演出の速さ（時間の倍率）を扱い、結果画面からの切り替えを行う
//!         対局の進行（手番・CPU・人間の入力・公開と判定の演出）はシステムが、音や粒子は出来事を受けた
//!         MatchFeedbackSystem が、画面 UI の書き換えは出来事を受けた各 UI が行います。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/Match.hpp>
#include <ChronoSort/UI/CardAlbum.hpp>
#include <ChronoSort/UI/DeskHud.hpp>
#include <ChronoSort/UI/OptionsSheet.hpp>
#include <ChronoSort/UI/PauseSheet.hpp>
#include <ChronoSort/UI/PlayPopups.hpp>
#include <ChronoSort/UI/ResultSheet.hpp>
#include <ChronoSort/UI/TutorialGuide.hpp>

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

        // 対局の状態・卓・粒子はレジストリのコンテキストが持つ（シーンと一緒に片付く）。
        // 画面 UI は出来事を購読するので、シーン（EventBus）より先に壊れるここのメンバに持つ
        Rules::MatchConfig                m_config;     // 対局の設定（もう一度のときに使う）
        std::unique_ptr<UI::DeskHud>      m_desk;       // 机の上の紙片
        std::unique_ptr<UI::PlayPopups>   m_popups;     // 判定の朱印と得点
        std::unique_ptr<UI::ResultSheet>  m_result;     // 結果の羊皮紙
        std::unique_ptr<UI::CardAlbum>    m_album;      // 出たカードを見る紙（結果の羊皮紙から開く）
        std::unique_ptr<UI::PauseSheet>   m_pause;      // 一時停止の紙
        std::unique_ptr<UI::OptionsSheet> m_options;    // 設定の紙（一時停止から開く）
        std::unique_ptr<UI::TutorialGuide> m_tutorial;  // 慣れるまでの対局で出す、遊び方の説明と手本
    };

}    // namespace ChronoSort::Scene
