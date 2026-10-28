//----------------------------------------------------------------------------
//! @file   GameHud.hpp
//! @brief  対局画面の画面 UI（研究者の机に散らばった紙片）
//! @detail 得点は封蝋、ライフはインク壺、山札の残りはピン留めの札、手番はしおりのリボン。
//!         判定の朱印・得点の浮き上がり・通知の紙片・結果の羊皮紙もここで出します。
//!         値は Rules::Match を毎フレーム読んで反映し、変わった瞬間に弾ませます。
//!         常に出しておく紙片の置き方（位置・大きさ・傾き・揺れ）は Data/UI/GameHud.json に書き、ここでは文字と動きを付けます。
//!         しばらく見せて消す紙片は PaperUI::DestroyAfter（LifetimeSystem）に片付けさせます。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/Match.hpp>
#include <ChronoSort/UI/PaperLayout.hpp>
#include <ChronoSort/UI/PaperUI.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ChronoSort::UI {

    //! 結果画面で選ばれた行動です。
    enum class ResultChoice {
        None,     // まだ選ばれていない
        Retry,    // もう一度
        Title,    // タイトルへ
    };

    //! 一時停止の紙で選ばれた行動です。
    enum class PauseChoice {
        None,       // まだ選ばれていない
        Resume,     // 続ける
        Options,    // 設定
        Title,      // タイトルへ
    };

    //! 対局画面の画面 UI です。
    class GameHud {
    public:

        //! 作り先のシーン・アセット管理・表示する対局を受け取ります。
        //! @param  [in] scene        UI を作るシーン。
        //! @param  [in] assetManager アセット管理。
        //! @param  [in] match        表示する対局。
        GameHud(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager, const Rules::Match& match);

        //! 常に出しておく UI を作ります。
        void Build();

        //! 値の反映と、時間切れの UI の片付けを行います。
        //! @param  [in] deltaTime 経過時間（秒）。
        void Update(float deltaTime);

        //! 手番のプレイヤーを示します（しおりのリボンを移す）。
        //! @param  [in] player プレイヤー番号。
        void SetTurn(size_t player);

        //! 人間が操作していない時間を伝えます（しばらく放置されたら次の行動を促す）。
        //! @param  [in] seconds 最後に操作してからの秒数（人間の手番でなければ 0）。
        void SetIdleSeconds(float seconds);

        //! 通知の紙片を滑り込ませ、少し置いてから抜きます。
        //! @param  [in] text 文字列。
        //! @param  [in] big  大きく出すなら true。
        void ShowBanner(const std::wstring& text, bool big);

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

        //! 1枚の判定が終わったことを伝えます（操作説明を消すなど）。
        //! @param  [in] player プレイヤー番号。
        //! @param  [in] result 置いた結果。
        void OnPlayResolved(size_t player, const Rules::PlayResult& result);

        //! 結果の羊皮紙を出します。
        void ShowResult();

        //! 結果に「新記録」の印を添えるかを設定します（ShowResult の前に呼ぶ）。
        //! @param  [in] newRecord 新記録なら true。
        void SetNewRecord(bool newRecord) { m_newRecord = newRecord; }

        //! 結果画面で選ばれた行動を返します。
        //! @return 選ばれた行動。
        ResultChoice PollResultChoice();

        //! 一時停止の紙を出します（出ていれば何もしません）。
        void OpenPause();

        //! 一時停止の紙を引っ込めます。
        void ClosePause();

        //! 一時停止の紙が出ているかを返します。
        //! @return 出ていれば true。
        bool IsPauseOpen() const { return m_pause != entt::null; }

        //! 一時停止の紙で選ばれた行動を返します。
        //! @return 選ばれた行動。
        PauseChoice PollPauseChoice();

    private:

        //! 指定した座標に貼り付く（カード追従の）UI を作ります。
        //! @param  [in] card        追従するカード。
        //! @param  [in] worldOffset カードからのずれ（ワールド）。
        //! @return 作成したエンティティ。
        Tsukino::ECS::Entity CreateAnchor(Tsukino::ECS::Entity card, const hlslpp::float3& worldOffset);

        //! ライフのインク壺を更新します（減ったら倒して空にする）。
        void RefreshLives();

        Tsukino::ECS::Scene&                             m_scene;           // UI を作るシーン
        PaperUI                                          m_ui;              // 紙片の部品
        const Rules::Match&                              m_match;           // 表示する対局
        PaperLayout                                      m_layout;          // 常に出しておく紙片の置き方（Data/UI/GameHud.json）

        Tsukino::ECS::Entity              m_seal       = entt::null;    // 封蝋
        Tsukino::ECS::Entity              m_sealText   = entt::null;    // 封蝋の数字
        Tsukino::ECS::Entity              m_sealLabel  = entt::null;    // 封蝋の札の文字
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
        float                             m_displayScore = 0.0f;        // 数え上げ中の得点表示
        int                               m_shownRound   = 0;           // 表示しているラウンド
        size_t                            m_turnPlayer   = SIZE_MAX;    // 強調している手番のプレイヤー
        bool                              m_lifeWarning  = false;       // 最後のインク壺を震わせているか
        bool                              m_deckWarning  = false;       // 山札の札を弾ませているか
        Tsukino::ECS::Entity              m_nudged       = entt::null;  // 放置の催促で震わせている UI

        // 結果画面
        // 一時停止
        Tsukino::ECS::Entity m_pause        = entt::null;    // 一時停止の紙
        Tsukino::ECS::Entity m_pauseResume  = entt::null;    // 続ける
        Tsukino::ECS::Entity m_pauseOptions = entt::null;    // 設定
        Tsukino::ECS::Entity m_pauseTitle   = entt::null;    // タイトルへ

        Tsukino::ECS::Entity m_result      = entt::null;
        Tsukino::ECS::Entity m_retryButton = entt::null;
        Tsukino::ECS::Entity m_titleButton = entt::null;
        Tsukino::ECS::Entity m_resultScore = entt::null;    // 数え上げる得点の文字
        float                m_resultTime  = -1.0f;         // 結果を出してからの時間
        bool                 m_newRecord   = false;         // 新記録か
    };

}    // namespace ChronoSort::UI
