//----------------------------------------------------------------------------
//! @file   TitleScene.hpp
//! @brief  タイトル画面
//! @detail カードが散らばった机を背景に、ピンやテープで留めたメモ紙をメニューとして並べます。
//!         CPU 対戦を選ぶと、人数と強さを選ぶ紙が机の上に置かれます。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/Match.hpp>
#include <ChronoSort/Scene/DeskStage.hpp>
#include <ChronoSort/UI/PaperUI.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>

#include <memory>
#include <vector>

namespace ChronoSort::Scene {

    //! タイトル画面です。
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

        //! メニューのメモ紙を1枚作ります。
        //! @param  [in] label    見出し。
        //! @param  [in] note     補足（空なら出さない）。
        //! @param  [in] center   位置。
        //! @param  [in] angle    傾き（度）。
        //! @param  [in] delay    滑り込む遅れ（秒）。
        //! @param  [in] fastener 留め具（"pin.png" か "tape.png"）。
        //! @return 作成した紙片。
        Tsukino::ECS::Entity CreateMenuNote(const std::wstring& label, const std::wstring& note, const hlslpp::float2& center, float angle, float delay,
                                            const char* fastener);

        //! CPU 対戦の設定の紙を開きます。
        void OpenVersusSheet();

        //! CPU 対戦の設定の紙を閉じます。
        void CloseVersusSheet();

        //! 設定の紙の選択肢の見た目を、今の選択に合わせます。
        void RefreshVersusChoices();

        std::unique_ptr<UI::PaperUI> m_ui;       // 紙片の部品
        DeskStage                    m_stage;    // 背景の机

        Tsukino::ECS::Entity m_solo    = entt::null;
        Tsukino::ECS::Entity m_versus  = entt::null;
        Tsukino::ECS::Entity m_credits = entt::null;
        Tsukino::ECS::Entity m_quit    = entt::null;

        // CPU 対戦の設定
        Tsukino::ECS::Entity              m_sheet = entt::null;    // 設定の紙
        std::vector<Tsukino::ECS::Entity> m_countTags;             // 人数の札
        std::vector<Tsukino::ECS::Entity> m_levelTags;             // 強さの札
        std::vector<Tsukino::ECS::Entity> m_countTexts;            // 人数の札の文字
        std::vector<Tsukino::ECS::Entity> m_levelTexts;            // 強さの札の文字
        Tsukino::ECS::Entity              m_start  = entt::null;   // 始める
        Tsukino::ECS::Entity              m_cancel = entt::null;   // やめる
        int                               m_cpuCount = 2;          // 選んだ CPU の人数
        Rules::CpuLevel                   m_cpuLevel = Rules::CpuLevel::Normal;    // 選んだ強さ
        std::vector<Tsukino::ECS::Entity> m_hovered;               // 前のフレームでカーソルが重なっていた紙片
    };

}    // namespace ChronoSort::Scene
