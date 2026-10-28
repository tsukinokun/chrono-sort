//----------------------------------------------------------------------------
//! @file   DeskHud.hpp
//! @brief  対局画面の机の上に常に出しておく紙片（研究者の机に散らばった紙片）
//! @detail 得点は封蝋、ライフはインク壺、山札の残りはピン留めの札、手番はしおりのリボン、名札、操作説明、
//!         連続正解の札、通知の帯です。対局の出来事（MatchEvents）を受けて書き換え、変わった瞬間に弾ませます。
//!         毎フレームの更新は持ちません（得点の数え上げは CountUpTextSystem が行う）。
//!         紙片の置き方（位置・大きさ・傾き・揺れ）は Data/UI/GameHud.json に書き、ここでは文字と動きを付けます。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/Match.hpp>
#include <ChronoSort/UI/PaperLayout.hpp>
#include <ChronoSort/UI/PaperUI.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Core/ECS/Event/ScopedConnection.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace Tsukino::ECS {
    class EventBus;
}    // namespace Tsukino::ECS

namespace ChronoSort::UI {

    //! 対局画面の机の上に常に出しておく紙片です。
    class DeskHud {
    public:

        //! 作り先のシーン・アセット管理・表示する対局を受け取り、出来事の購読を始めます。
        //! @param  [in] scene        UI を作るシーン（このシーンの EventBus を購読する）。
        //! @param  [in] assetManager アセット管理。
        //! @param  [in] match        表示する対局。
        DeskHud(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager, const Rules::Match& match);

        //! 紙片を作ります。
        void Build();

    private:

        //! 規則の値（ラウンド・ライフ・山札・手札の枚数）を表示に合わせます。
        void Refresh();

        //! ライフのインク壺を更新します（減ったら倒して空にする）。
        void RefreshLives();

        //! 手番のプレイヤーを示します（しおりのリボンを移す）。
        //! @param  [in] player プレイヤー番号。
        void SetTurn(size_t player);

        //! 放置されているあいだ、次の行動を促します（操作説明か手番のリボンを震わせる）。
        //! @param  [in] active 促すなら true、操作が戻ったら false。
        void SetNudge(bool active);

        //! 通知の帯を滑り込ませ、少し置いてから抜きます。
        //! @param  [in] text 文字列。
        //! @param  [in] big  大きく出すなら true。
        void ShowBanner(const std::wstring& text, bool big);

        //! 1枚の判定が終わったときの表示（操作説明を片付ける・封蝋を弾ませる・連続正解の札）。
        //! @param  [in] player プレイヤー番号。
        //! @param  [in] result 置いた結果。
        void OnPlayResolved(size_t player, const Rules::PlayResult& result);

        Tsukino::ECS::Scene&                        m_scene;           // UI を作るシーン
        PaperUI                                     m_ui;              // 紙片の部品
        const Rules::Match&                         m_match;           // 表示する対局
        PaperLayout                                 m_layout;          // 紙片の置き方（Data/UI/GameHud.json）
        std::vector<Tsukino::ECS::ScopedConnection> m_connections;     // 出来事の購読

        Tsukino::ECS::Entity              m_seal       = entt::null;    // 封蝋
        Tsukino::ECS::Entity              m_sealText   = entt::null;    // 封蝋の数字
        std::vector<Tsukino::ECS::Entity> m_inks;                       // インク壺
        int                               m_shownLives = 0;             // 表示しているライフ
        Tsukino::ECS::Entity              m_deckTag    = entt::null;    // 山札の札
        Tsukino::ECS::Entity              m_deckText   = entt::null;    // 山札の残り枚数
        size_t                            m_shownDeck  = 0;             // 表示している山札の残り
        Tsukino::ECS::Entity              m_streakTag  = entt::null;    // 連続正解の札
        Tsukino::ECS::Entity              m_streakText = entt::null;    // 連続正解の文字
        Tsukino::ECS::Entity              m_hint       = entt::null;    // 操作説明
        Tsukino::ECS::Entity              m_ribbon     = entt::null;    // 手番のリボン
        std::vector<Tsukino::ECS::Entity> m_playerTags;                 // 各プレイヤーの名札
        std::vector<Tsukino::ECS::Entity> m_handCounts;                 // 各プレイヤーの手札枚数の文字
        std::vector<int>                  m_shownHandCounts;            // 表示している手札枚数（脱落は -1）
        int                               m_shownRound  = 0;            // 表示しているラウンド
        size_t                            m_turnPlayer  = SIZE_MAX;     // 強調している手番のプレイヤー
        bool                              m_lifeWarning = false;        // 最後のインク壺を震わせているか
        bool                              m_deckWarning = false;        // 山札の札を弾ませているか
        Tsukino::ECS::Entity              m_nudged      = entt::null;   // 放置の催促で震わせている UI
    };

}    // namespace ChronoSort::UI
