//----------------------------------------------------------------------------
//! @file   LoadingScene.hpp
//! @brief  読込画面
//! @detail 起動直後に一度だけ挟み、ゲームで使うアセットを裏スレッドで読み込みます。
//!         読み終えたテクスチャは本体スレッドで少しずつ GPU へ転送し、
//!         すべて終わったら次のシーンへ進みます。以降の画面切替や操作では、読み込みで止まりません。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/UI/PaperUI.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/Engine/Asset/AssetLoadQueue.hpp>
#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>

#include <deque>
#include <functional>
#include <memory>

namespace ChronoSort::Scene {

    //! 読込画面です。
    class LoadingScene : public Tsukino::EngineIntegration::GameSceneBase {
    public:

        //! 読み終えたあとに進むシーンを作る関数を受け取ります。
        //! @param  [in] next 次のシーンを作る関数。
        explicit LoadingScene(std::function<std::unique_ptr<Tsukino::EngineIntegration::GameSceneBase>()> next);
        ~LoadingScene() override = default;

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

        //! 読み終えたテクスチャを、時間の予算の範囲で GPU へ転送します。
        //! @param  [in] budgetSeconds 1フレームで使ってよい時間（秒）。
        void UploadTextures(float budgetSeconds);

        std::function<std::unique_ptr<Tsukino::EngineIntegration::GameSceneBase>()> m_next;    // 次のシーン
        std::unique_ptr<Tsukino::Asset::AssetLoadQueue> m_queue;                   // 裏読みのキュー
        std::unique_ptr<UI::PaperUI>                    m_ui;                      // 紙片の部品
        std::deque<Tsukino::Asset::AssetHandle>         m_pendingUploads;          // GPU へ転送待ちのアセット
        size_t                                          m_uploaded   = 0;          // 転送済みの数
        float                                           m_elapsed    = 0.0f;       // 表示してからの時間
        float                                           m_shown      = 0.0f;       // 表示している進み具合（なめらかに追う）
        Tsukino::ECS::Entity                            m_line       = entt::null; // 進み具合の墨の線
        Tsukino::ECS::Entity                            m_percent    = entt::null; // 進み具合の数字
        bool                                            m_leaving    = false;      // 次へ進んだか
    };

}    // namespace ChronoSort::Scene
