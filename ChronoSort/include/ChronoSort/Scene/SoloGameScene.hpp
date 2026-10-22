//----------------------------------------------------------------------------
//! @file   SoloGameScene.hpp
//! @brief  ソロモードの対局画面
//! @detail 透視カメラで見下ろす 3D の卓に、場の列と扇状の手札を並べます。
//!         現在は描画の土台の段階で、操作とターン進行はこれから載せます。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Board/CardFactory.hpp>
#include <ChronoSort/Rules/Deck.hpp>
#include <ChronoSort/Rules/Timeline.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>

#include <memory>
#include <random>
#include <vector>

namespace ChronoSort::Scene {

    //! ソロモードの対局画面です。
    class SoloGameScene : public Tsukino::EngineIntegration::GameSceneBase {
    public:
        SoloGameScene()           = default;
        ~SoloGameScene() override = default;

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

        //! 描画と演出に使うシステムを登録します。
        void RegisterSystems();

        //! カメラ・ライト・後処理を配置します。
        void CreateStage();

        //! 手札の各カードの目標姿勢を、扇状の並びに合わせて設定します。
        void LayoutHand();

        std::unique_ptr<Board::CardFactory> m_cardFactory;                 // カードの生成
        Rules::Deck                         m_deck;                        // 山札
        Rules::Timeline                     m_timeline;                    // 場の列（判定用）
        std::mt19937                        m_rng;                         // シャッフル用の乱数
        std::vector<Tsukino::ECS::Entity>   m_handCards;                   // 手札のカード（左から順）
        Tsukino::ECS::Entity                m_firstCard = entt::null;      // 場の最初のカード
        bool                                m_firstCardRevealed = true;    // 最初のカードが年号面を上にしているか
    };

}    // namespace ChronoSort::Scene
