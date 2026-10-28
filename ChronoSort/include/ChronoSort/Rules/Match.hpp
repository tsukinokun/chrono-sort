//----------------------------------------------------------------------------
//! @file   Match.hpp
//! @brief  1回の対局の進行
//! @detail 山札・場・捨て札・各プレイヤーの手札と、手番の巡回や勝敗判定を持ちます。
//!         ECS や描画には依存せず、画面側は Play() の結果を見て演出だけを行います。
//!
//!         ソロ   : 手札を常に kSoloHandSize 枚に保ち、間違えるとライフが減る。
//!                  ライフが尽きるとゲームオーバー、山札と手札を出し切るとクリア。
//!         対戦   : 間違えたカードは捨て札にして1枚引く。
//!                  ラウンドの終わりに手札を出し切った人がいれば勝ち。複数なら延長戦。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/CardData.hpp>
#include <ChronoSort/Rules/Deck.hpp>
#include <ChronoSort/Rules/Timeline.hpp>

#include <optional>
#include <random>
#include <string>
#include <vector>

namespace ChronoSort::Rules {

    //! 対局の種類です。
    enum class MatchMode {
        Solo,      // 1人でライフ制
        Versus,    // CPU と対戦
    };

    //! CPU の強さです。
    enum class CpuLevel {
        Easy,
        Normal,
        Hard,
    };

    //! 対局の参加者です。
    struct Player {
        std::string           name;                         // 表示名（UTF-8）
        bool                  isCpu      = false;           // CPU が操作するか
        CpuLevel              cpuLevel   = CpuLevel::Normal;    // CPU の強さ
        std::vector<CardData> hand;                         // 手札
        bool                  eliminated = false;           // 延長戦で脱落したか
        int                   correctCount = 0;             // 正しく置けた枚数
    };

    //! 対局の設定です。
    struct MatchConfig {
        MatchMode             mode = MatchMode::Solo;    // 対局の種類
        std::vector<CpuLevel> cpus;                      // 対戦の CPU（1〜3人）
        std::string           playerName = "あなた";     // 人間のプレイヤー名
    };

    //! カードを1枚置いた結果です。
    struct PlayResult {
        CardData            card;                    // 置いたカード
        size_t              slot           = 0;      // 置こうとしたスロット
        bool                correct        = false;  // 正しい位置だったか
        std::vector<size_t> correctSlots;            // 本来正しかったスロット（置く前の場の基準）
        int                 scoreGained    = 0;      // ソロで得た得点
        int                 streak         = 0;      // 置いた後の連続正解数
        bool                closeCall      = false;  // 隣のカードと年代が近かったか（ボーナス対象）
        std::optional<CardData> drawnCard;           // 補充・ペナルティで引いたカード
        bool                lifeLost       = false;  // ソロでライフを失ったか
    };

    //! 対局の終わり方です。
    enum class MatchOutcome {
        InProgress,    // 続行中
        SoloCleared,   // ソロで出し切った
        SoloFailed,    // ソロでライフが尽きた
        VersusWon,     // 対戦で勝者が決まった
    };

    constexpr int kSoloHandSize = 3;    // ソロで保つ手札の枚数

    //! 1回の対局の進行です。
    class Match {
    public:

        //! 対局を始めます。山札をシャッフルし、場の1枚目と手札を配ります。
        //! @param  [in]     config 対局の設定。
        //! @param  [in]     cards  使うカードの一覧。
        //! @param  [in,out] rng    乱数生成器。
        void Start(const MatchConfig& config, const std::vector<CardData>& cards, std::mt19937& rng);

        //! 現在の手番のプレイヤーが、手札のカードを場に置きます。
        //! @param  [in] handIndex 手札の何枚目か。
        //! @param  [in] slot      置くスロット。
        //! @return 置いた結果。対局が終わっている・引数が不正なら std::nullopt。
        std::optional<PlayResult> Play(size_t handIndex, size_t slot);

        //! 手番を次のプレイヤーへ進めます。ラウンドの終わりなら勝敗を判定します。
        //! Play() のあと、画面側の演出が終わったら呼びます。
        void AdvanceTurn();

        //! 現在の手番のプレイヤー番号を返します。
        //! @return プレイヤー番号。
        size_t CurrentPlayerIndex() const { return m_current; }

        //! プレイヤーの一覧を返します。0 番が人間です。
        //! @return プレイヤーの一覧。
        const std::vector<Player>& Players() const { return m_players; }

        //! 場の列を返します。
        //! @return 場の列。
        const Timeline& GetTimeline() const { return m_timeline; }

        //! 捨て札を返します。
        //! @return 捨て札（古い順）。
        const std::vector<CardData>& Discards() const { return m_discards; }

        //! 山札の残り枚数を返します。
        //! @return 残り枚数。
        size_t DeckRemaining() const { return m_deck.Remaining(); }

        //! 対局の状況を返します。
        //! @return 続行中か、どう終わったか。
        MatchOutcome Outcome() const { return m_outcome; }

        //! 勝者のプレイヤー番号を返します（対戦で勝者が決まったときだけ有効）。
        //! @return 勝者の番号。
        size_t Winner() const { return m_winner; }

        //! 延長戦の最中かを返します。
        //! @return 延長戦なら true。
        bool IsTiebreak() const { return m_tiebreak; }

        //! 設定を返します。
        //! @return 対局の設定。
        const MatchConfig& Config() const { return m_config; }

        int Score() const { return m_score; }              //!< ソロの得点を返します。
        int Lives() const { return m_lives; }              //!< ソロの残りライフを返します。
        int Streak() const { return m_streak; }            //!< 現在の連続正解数を返します。
        int BestStreak() const { return m_bestStreak; }    //!< 最長の連続正解数を返します。
        int Round() const { return m_round; }              //!< 対戦の現在のラウンド（1 始まり）を返します。

    private:

        //! 山札から1枚引きます。山札が尽きていたら捨て札を混ぜ直してから引きます。
        //! @return 引いたカード。どこにも残っていなければ std::nullopt。
        std::optional<CardData> DrawCard();

        //! ソロの得点を計算します。
        //! @param  [in] closeCall 隣と年代が近かったか。
        //! @return 得点。
        int CalculateSoloScore(bool closeCall) const;

        //! ラウンドの終わりに勝敗を判定し、延長戦なら追加の手札を配ります。
        void ResolveRoundEnd();

        //! 次に手番が回る、脱落していないプレイヤーを探します。
        //! @param  [in] from 探し始める番号（この番号は含まない）。
        //! @return プレイヤー番号。
        size_t NextActivePlayer(size_t from) const;

        MatchConfig           m_config;                               // 対局の設定
        Deck                  m_deck;                                 // 山札
        Timeline              m_timeline;                             // 場の列
        std::vector<CardData> m_discards;                             // 捨て札
        std::vector<Player>   m_players;                              // 参加者（0 番が人間）
        std::mt19937*         m_rng        = nullptr;                 // 捨て札を混ぜ直す乱数
        size_t                m_current    = 0;                       // 手番のプレイヤー
        size_t                m_winner     = 0;                       // 勝者
        MatchOutcome          m_outcome    = MatchOutcome::InProgress;    // 対局の状況
        bool                  m_tiebreak   = false;                   // 延長戦の最中か
        int                   m_round      = 1;                       // 対戦のラウンド
        int                   m_score      = 0;                       // ソロの得点
        int                   m_lives      = 0;                       // ソロの残りライフ
        int                   m_streak     = 0;                       // 連続正解数
        int                   m_bestStreak = 0;                       // 最長の連続正解数
    };

}    // namespace ChronoSort::Rules
