//----------------------------------------------------------------------------
//! @file   CreditsScene.hpp
//! @brief  出典・クレジット画面
//! @detail Credits.json（カードの絵と BGM の出典・作者・ライセンス）を、
//!         研究ノートの頁のようにめくって読めるように表示します。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Scene/DeskStage.hpp>
#include <ChronoSort/UI/PaperUI.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>

#include <memory>
#include <string>
#include <vector>

namespace ChronoSort::Scene {

    //! 出典・クレジット画面です。
    class CreditsScene : public Tsukino::EngineIntegration::GameSceneBase {
    public:
        CreditsScene()           = default;
        ~CreditsScene() override = default;

        //! シーンを更新します。
        //! @param  [in] api       エンジン API。
        //! @param  [in] deltaTime 経過時間（秒）。
        void OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) override;

        //! シーンの終了処理を行います。
        void OnExit() override;

    private:

        //! 1行分の表示内容です。
        struct Line {
            std::wstring text;           // 文字列
            bool         heading = false;    // 見出しか
            bool         faded   = false;    // 補足（薄墨）か
        };

        //! シーンを初期化します。
        //! @param  [in] api エンジン API。
        void OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) override;

        //! Credits.json などから表示する行を作ります。
        void LoadLines();

        //! 指定した頁を表示します。
        //! @param  [in] page      頁番号。
        //! @param  [in] direction めくる向き（1 で次へ、-1 で前へ、0 で最初の表示）。
        void ShowPage(int page, int direction);

        std::unique_ptr<UI::PaperUI> m_ui;                     // 紙片の部品
        DeskStage                    m_stage;                  // 背景の机
        std::vector<Line>            m_lines;                  // 全行
        Tsukino::ECS::Entity         m_sheet    = entt::null;  // 羊皮紙
        Tsukino::ECS::Entity         m_page     = entt::null;  // 今の頁の文字のまとめ
        Tsukino::ECS::Entity         m_pageNo   = entt::null;  // 頁番号
        Tsukino::ECS::Entity         m_prev     = entt::null;  // 前の頁
        Tsukino::ECS::Entity         m_next     = entt::null;  // 次の頁
        Tsukino::ECS::Entity         m_back     = entt::null;  // 戻る
        int                          m_current  = 0;           // 今の頁
        int                          m_pageCount = 1;          // 頁数
    };

}    // namespace ChronoSort::Scene
