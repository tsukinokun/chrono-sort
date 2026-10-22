//----------------------------------------------------------------------------
//! @file   TitleScene.hpp
//! @brief  タイトル画面
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/Deck.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>

#include <random>
#include <string>

namespace ChronoSort::Scene {

    //! タイトル画面です。
    //! 現時点ではデッキの読み込みと日本語表示の確認を兼ねています。
    class TitleScene : public Tsukino::EngineIntegration::GameSceneBase {
    public:
        TitleScene()           = default;
        ~TitleScene() override = default;

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

        //! 画面にテキストを1つ置きます。
        //! @param  [in] text  表示する文字列。
        //! @param  [in] x     画面上の X 座標（ピクセル）。
        //! @param  [in] y     画面上の Y 座標（ピクセル）。
        //! @param  [in] scale 文字の拡大率。
        //! @return 作成したエンティティ。
        Tsukino::ECS::Entity CreateText(const std::wstring& text, float x, float y, float scale);

        //! 山札から1枚引き、見本カードの表示を差し替えます。
        void ShowNextSampleCard();

        Rules::Deck          m_deck;                  // 見本表示用の山札（日本史＋世界史）
        std::mt19937         m_rng;                   // シャッフル用の乱数
        Tsukino::ECS::Entity m_sampleTitle = entt::null;    // 見本カードの出来事名
        Tsukino::ECS::Entity m_sampleYear  = entt::null;    // 見本カードの年号
    };

}    // namespace ChronoSort::Scene
