//----------------------------------------------------------------------------
//! @file   TitleScene.hpp
//! @brief  タイトル画面
//! @detail カードが散らばった机を背景に、ピンやテープで留めたメモ紙をメニューとして並べます。
//!         題字の下のしおり3枚で出題の範囲（日本史・世界史・通史）を選び、選んだ範囲は記録に残して次回も使います。
//!         CPU 対戦を選ぶと、人数と強さを選ぶ紙が机の上に置かれます。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Game/SaveData.hpp>
#include <ChronoSort/Rules/Match.hpp>
#include <ChronoSort/Scene/DeskStage.hpp>
#include <ChronoSort/UI/OptionsSheet.hpp>
#include <ChronoSort/UI/PaperLayout.hpp>
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
        //! @param  [in] layout   置き方の名前（Data/UI/Title.json）。
        //! @param  [in] delay    滑り込む遅れ（秒）。
        //! @param  [in] fastener 留め具（"pin.png" か "tape.png"）。
        //! @return 作成した紙片。
        Tsukino::ECS::Entity CreateMenuNote(const std::wstring& label, const std::wstring& note, const std::string& layout, float delay, const char* fastener);

        //! CPU 対戦の設定の紙を開きます。
        void OpenVersusSheet();

        //! CPU 対戦の設定の紙を閉じます。
        void CloseVersusSheet();

        //! 設定の紙の選択肢の見た目を、今の選択に合わせます。
        void RefreshVersusChoices();

        //! 出題の範囲のしおりを3枚作ります。
        //! @param  [in] title 題字の紙片（「出題」の見出しを貼る）。
        void CreateRangeTabs(Tsukino::ECS::Entity title);

        //! しおりの持ち上がりを、選んでいる範囲へなめらかに寄せます。
        //! @param  [in] deltaTime 経過時間（秒）。
        void UpdateRangeTabs(float deltaTime);

        //! 出題の範囲を選び直します（見た目・記録のメモ・保存を更新する）。
        //! @param  [in] range 出題の範囲。
        void SelectRange(Rules::CardRange range);

        //! 記録のメモを、選んでいる範囲の記録に合わせます（記録が無ければ引っ込める）。
        //! @param  [in] slideDelay 出すときに滑り込み始めるまでの待ち（秒）。
        void RefreshRecordMemo(float slideDelay = 0.0f);

        std::unique_ptr<UI::PaperUI> m_ui;       // 紙片の部品
        UI::PaperLayout              m_layout;   // 紙片の置き方（Data/UI/Title.json）
        DeskStage                    m_stage;    // 背景の机
        Game::SaveData               m_save;     // 記録（出題の範囲を選び直すたびに保存する）

        // 出題の範囲
        Rules::CardRange                  m_range = Rules::CardRange::All;    // 選んでいる範囲
        std::vector<Rules::CardRange>     m_rangeOrder;            // しおりの並び順の範囲
        std::vector<Tsukino::ECS::Entity> m_rangeHolders;          // しおりを持ち上げるための親
        std::vector<Tsukino::ECS::Entity> m_rangeTags;             // しおりの紙片
        std::vector<Tsukino::ECS::Entity> m_rangeTexts;            // しおりの文字
        std::vector<hlslpp::float2>       m_rangeBases;            // しおりの置き場所
        std::vector<float>                m_rangeLift;             // しおりの今の持ち上がり（0〜1）
        Tsukino::ECS::Entity              m_rangeRibbon = entt::null;    // 選んでいるしおりに垂らす紐

        // 記録のメモ
        Tsukino::ECS::Entity              m_record      = entt::null;    // メモの紙片
        Tsukino::ECS::Entity              m_recordTitle = entt::null;    // 見出し
        std::vector<Tsukino::ECS::Entity> m_recordLines;           // 記録の行
        bool                              m_recordShown = false;   // 出しているか

        Tsukino::ECS::Entity m_solo    = entt::null;
        Tsukino::ECS::Entity m_versus  = entt::null;
        Tsukino::ECS::Entity m_credits = entt::null;
        Tsukino::ECS::Entity m_quit    = entt::null;
        Tsukino::ECS::Entity m_settings = entt::null;

        std::unique_ptr<UI::OptionsSheet> m_options;    // 設定の紙

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
