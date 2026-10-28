//----------------------------------------------------------------------------
//! @file   ResultSheet.hpp
//! @brief  対局の結果の羊皮紙
//! @detail ResultRequestedEvent を受けて、得点（ソロ）か勝者（対戦）とランクの印を書いた紙を落とします。
//!         新記録かどうかは MatchEndedEvent で受け取ります。得点の数え上げは CountUpTextSystem が行います。
//!         選ばれた行動は Poll で受け取ります（画面の切り替えは GameScene が行う）。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/Match.hpp>
#include <ChronoSort/UI/PaperLayout.hpp>
#include <ChronoSort/UI/PaperUI.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Core/ECS/Event/ScopedConnection.hpp>

#include <vector>

namespace ChronoSort::UI {

    //! 結果画面で選ばれた行動です。
    enum class ResultChoice {
        None,     // まだ選ばれていない
        Retry,    // もう一度
        Title,    // タイトルへ
    };

    //! 対局の結果の羊皮紙です。
    class ResultSheet {
    public:

        //! 作り先のシーン・アセット管理・表示する対局を受け取り、出来事の購読を始めます。
        //! @param  [in] scene        UI を作るシーン（このシーンの EventBus を購読する）。
        //! @param  [in] assetManager アセット管理。
        //! @param  [in] match        表示する対局。
        ResultSheet(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager, const Rules::Match& match);

        //! 選ばれた行動を返します（紙が落ちてくる途中のクリックは受け付けない）。
        //! @return 選ばれた行動。
        ResultChoice Poll();

    private:

        //! 結果の羊皮紙を出します。
        void Show();

        Tsukino::ECS::Scene&                        m_scene;          // UI を作るシーン
        PaperUI                                     m_ui;             // 紙片の部品
        const Rules::Match&                         m_match;          // 表示する対局
        PaperLayout                                 m_layout;         // 紙と札の置き方（Data/UI/GameHud.json）
        std::vector<Tsukino::ECS::ScopedConnection> m_connections;    // 出来事の購読

        Tsukino::ECS::Entity m_sheet       = entt::null;    // 羊皮紙
        Tsukino::ECS::Entity m_retryButton = entt::null;    // もう一度
        Tsukino::ECS::Entity m_titleButton = entt::null;    // タイトルへ
        bool                 m_newRecord   = false;         // 新記録か
        bool                 m_ready       = false;         // ボタンを受け付けるか
    };

}    // namespace ChronoSort::UI
