//----------------------------------------------------------------------------
//! @file   PlayPopups.hpp
//! @brief  判定したカードの上に出す朱印と得点
//! @detail PlayJudgedEvent を受けて、カードに貼り付く朱印（正・誤）と、浮き上がる得点を出します。
//!         どちらもしばらく見せたら LifetimeSystem が片付けます。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/Match.hpp>
#include <ChronoSort/UI/PaperLayout.hpp>
#include <ChronoSort/UI/PaperUI.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Core/ECS/Event/ScopedConnection.hpp>

#include <vector>

namespace ChronoSort::UI {

    //! 判定したカードの上に出す朱印と得点です。
    class PlayPopups {
    public:

        //! 作り先のシーン・アセット管理・表示する対局を受け取り、出来事の購読を始めます。
        //! @param  [in] scene        UI を作るシーン（このシーンの EventBus を購読する）。
        //! @param  [in] assetManager アセット管理。
        //! @param  [in] match        表示する対局。
        PlayPopups(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager, const Rules::Match& match);

    private:

        //! カードに貼り付く（カード追従の）UI を作ります。
        //! @param  [in] card        追従するカード。
        //! @param  [in] worldOffset カードからのずれ（ワールド）。
        //! @return 作成したエンティティ。
        Tsukino::ECS::Entity CreateAnchor(Tsukino::ECS::Entity card, const hlslpp::float3& worldOffset);

        //! カードの上に判定の朱印を押します。
        //! @param  [in] card    カード。
        //! @param  [in] correct 正解なら true（「正」）、不正解なら false（「誤」）。
        void ShowStamp(Tsukino::ECS::Entity card, bool correct);

        //! カードの上に得点を浮き上がらせます。
        //! @param  [in] card      カード。
        //! @param  [in] score     得点（0 なら得点は出さない）。
        //! @param  [in] closeCall 隣と年代が近かったか（「僅差！」を添える）。
        //! @param  [in] streak    連続正解数（2 以上で「連続」を添える）。
        void ShowScorePopup(Tsukino::ECS::Entity card, int score, bool closeCall, int streak);

        Tsukino::ECS::Scene&                        m_scene;          // UI を作るシーン
        PaperUI                                     m_ui;             // 紙片の部品
        const Rules::Match&                         m_match;          // 表示する対局
        PaperLayout                                 m_layout;         // 朱印の置き方（Data/UI/GameHud.json）
        std::vector<Tsukino::ECS::ScopedConnection> m_connections;    // 出来事の購読
    };

}    // namespace ChronoSort::UI
