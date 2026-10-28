//----------------------------------------------------------------------------
//! @file   GameScene.hpp
//! @brief  対局画面（ソロ・CPU 対戦）
//! @detail Rules::Match で対局を進め、BoardView で 3D の卓にカードを並べます。
//!         人間の手番はマウスでカードを掴んで（またはクリックで選んで）場の隙間へ置き、
//!         置いたカードは持ち上げ → 裏返し → 叩きつけ → 判定の順で演出します。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Board/BoardView.hpp>
#include <ChronoSort/Board/CardFactory.hpp>
#include <ChronoSort/Effects/ParticleEmitter.hpp>
#include <ChronoSort/Game/Sequencer.hpp>
#include <ChronoSort/Rules/Match.hpp>
#include <ChronoSort/UI/GameHud.hpp>
#include <ChronoSort/UI/OptionsSheet.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>

#include <hlsl++.h>

#include <memory>
#include <optional>
#include <random>

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

        //! 対局の段階です。
        enum class Phase {
            Dealing,      // 配っている
            HumanTurn,    // 人間の手番（入力待ち）
            CpuTurn,      // CPU が考えている
            Resolving,    // 置いたカードの公開と判定の演出中
            Finished,     // 対局が終わった
        };

        //! シーンを初期化します。
        //! @param  [in] api エンジン API。
        void OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) override;

        //! 描画と演出に使うシステムを登録します。
        void RegisterSystems();

        //! カメラ・ライト・後処理を配置します。
        void CreateStage();

        //! 場の1枚目と手札を、山札から順に配ります。
        void DealOpening();

        //! 手番を始めます。
        void StartTurn();

        //! 人間の手番の入力を処理します。
        //! @param  [in] deltaTime 経過時間（秒）。
        void UpdateHumanInput(float deltaTime);

        //! CPU の手番を進めます（考える演出のあとで置く）。
        void RunCpuTurn();

        //! 手札のカードを場に置き、公開と判定の演出を始めます。
        //! @param  [in] player    プレイヤー番号。
        //! @param  [in] handIndex 手札の番号。
        //! @param  [in] slot      置くスロット。
        void CommitPlay(size_t player, size_t handIndex, size_t slot);

        //! 置いたカードの公開・判定の演出を段取りします。
        //! @param  [in] player プレイヤー番号。
        //! @param  [in] entity 置いたカード。
        //! @param  [in] result 置いた結果。
        void RunReveal(size_t player, Tsukino::ECS::Entity entity, const Rules::PlayResult& result);

        //! 判定の演出のあと、手札の補充と手番の終わりを行います。
        void FinishPlay();

        //! 手札の見た目を Rules::Match の手札に合わせます（足りない分を山札から配る）。
        //! @param  [in] delay 配り始めるまでの待ち（秒）。
        //! @return 最後のカードが届くまでの時間（秒）。
        float SyncHands(float delay);

        //! 対局の終わりを演出し、結果を表示します。
        void ShowResult();

        //! 選択とドラッグの状態を解除し、手札へ戻します。
        void CancelSelection();

        //! カメラの揺れを更新します。
        //! @param  [in] deltaTime 経過時間（秒）。
        void UpdateCamera(float deltaTime);

        //! カメラを揺らします。
        //! @param  [in] strength 揺れの強さ。
        void ShakeCamera(float strength);

        //! マウスのレイを返します。
        //! @return ワールド空間のレイ。
        Tsukino::BuiltIn::ECS::CameraUtility::Ray MouseRay();

        Rules::MatchConfig                       m_config;                         // 対局の設定
        Rules::Match                             m_match;                          // 対局の進行
        std::mt19937                             m_rng;                            // 乱数
        std::unique_ptr<Board::CardFactory>      m_factory;                        // カードの生成
        std::unique_ptr<Board::BoardView>        m_board;                          // 卓の並べ方
        std::unique_ptr<Effects::ParticleEmitter> m_particles;                     // 粒子の演出
        std::unique_ptr<UI::GameHud>             m_hud;                            // 画面 UI
        std::unique_ptr<UI::OptionsSheet>        m_options;                        // 設定の紙（一時停止から開く）
        Game::Sequencer                          m_sequencer;                      // 演出の段取り
        Phase                                    m_phase = Phase::Dealing;         // 対局の段階
        Tsukino::EngineIntegration::EngineAPI*   m_api   = nullptr;                // シーン切り替え用

        // 入力
        int                   m_grabbed   = -1;          // 押している手札（ドラッグ前を含む）
        int                   m_selected  = -1;          // クリックで選んだ手札
        bool                  m_dragging  = false;       // ドラッグ中か
        hlslpp::float2        m_pressPoint = hlslpp::float2(0.0f, 0.0f);    // 押した画面位置
        hlslpp::float3        m_dragPoint  = hlslpp::float3(0.0f, 0.0f, 0.0f);    // ドラッグ中のカードの位置
        hlslpp::float3        m_dragVelocity = hlslpp::float3(0.0f, 0.0f, 0.0f);  // ドラッグの速度（傾きに使う）
        std::optional<size_t> m_preview;                 // 置こうとしているスロット
        float                 m_idleTime  = 0.0f;        // 人間の手番で操作していない時間（秒）
        hlslpp::float2        m_lastMouse = hlslpp::float2(0.0f, 0.0f);    // 前フレームのマウス位置

        // カメラ
        Tsukino::ECS::Entity m_camera      = entt::null;
        hlslpp::float3       m_cameraBasePosition = hlslpp::float3(0.0f, 0.0f, 0.0f);    // Prefab で決めたカメラの位置
        hlslpp::float3       m_cameraBaseLookAt   = hlslpp::float3(0.0f, 0.0f, 0.0f);    // Prefab で決めた注視点
        float                m_shake       = 0.0f;    // 揺れの強さ
        float                m_time        = 0.0f;    // 経過時間
        bool                 m_wasTiebreak = false;   // 直前まで延長戦だったか
    };

}    // namespace ChronoSort::Scene
