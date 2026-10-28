//----------------------------------------------------------------------------
//! @file   Match.cpp
//! @brief  1回の対局の進行の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Rules/Match.hpp>


#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iterator>

namespace ChronoSort::Rules {

    //----------------------------------------------------------------------------
    //! 対局を始めます。
    //----------------------------------------------------------------------------
    void Match::Start(const MatchConfig& config, const std::vector<CardData>& cards, std::mt19937& rng) {
        m_config     = config;
        m_rng        = &rng;
        m_deck       = Deck();
        m_discards.clear();
        m_players.clear();
        m_current    = 0;
        m_winner     = 0;
        m_outcome    = MatchOutcome::InProgress;
        m_tiebreak   = false;
        m_round      = 1;
        m_score      = 0;
        m_lives      = config.rules.soloLives;
        m_streak     = 0;
        m_bestStreak = 0;

        for(const CardData& card : cards) {
            m_deck.Add(card);
        }
        m_deck.Shuffle(rng);

        // 参加者。0 番は必ず人間
        Player human;
        human.name = config.playerName;
        m_players.push_back(human);
        if(config.mode == MatchMode::Versus) {
            for(size_t i = 0; i < config.cpus.size() && i < config.rules.cpus.size(); ++i) {
                Player cpu;
                cpu.name     = config.rules.cpus[i].name;
                cpu.portrait = config.rules.cpus[i].portrait;
                cpu.isCpu    = true;
                cpu.cpuLevel = config.cpus[i];
                m_players.push_back(cpu);
            }
        }

        if(const auto first = DrawCard()) {
            m_timeline.Reset(*first);
        }

        const int handSize = (config.mode == MatchMode::Solo) ? config.rules.soloHandSize : config.rules.InitialHandSize(static_cast<int>(m_players.size()));
        for(int i = 0; i < handSize; ++i) {
            for(Player& player : m_players) {
                if(const auto card = DrawCard()) {
                    player.hand.push_back(*card);
                }
            }
        }
    }

    //----------------------------------------------------------------------------
    //! 現在の手番のプレイヤーが、手札のカードを場に置きます。
    //----------------------------------------------------------------------------
    std::optional<PlayResult> Match::Play(size_t handIndex, size_t slot) {
        if(m_outcome != MatchOutcome::InProgress) {
            return std::nullopt;
        }
        Player& player = m_players[m_current];
        if(handIndex >= player.hand.size() || slot >= m_timeline.SlotCount()) {
            return std::nullopt;
        }

        PlayResult result;
        result.card         = player.hand[handIndex];
        result.slot         = slot;
        result.correctSlots = m_timeline.FindCorrectSlots(result.card);
        player.hand.erase(player.hand.begin() + static_cast<std::ptrdiff_t>(handIndex));

        if(m_timeline.TryPlace(slot, result.card)) {
            result.correct = true;
            ++player.correctCount;
            m_streak     = m_streak + 1;
            m_bestStreak = std::max(m_bestStreak, m_streak);

            // 置いた位置の両隣との年代差で「惜しい所を当てた」かを見る
            const auto& cards = m_timeline.Cards();
            int         nearest = INT32_MAX;
            if(slot > 0) {
                nearest = std::min(nearest, std::abs(cards[slot].year - cards[slot - 1].year));
            }
            if(slot + 1 < cards.size()) {
                nearest = std::min(nearest, std::abs(cards[slot].year - cards[slot + 1].year));
            }
            result.closeCall = nearest <= m_config.rules.closeCallYears;

            if(m_config.mode == MatchMode::Solo) {
                result.scoreGained = CalculateSoloScore(result.closeCall);
                m_score += result.scoreGained;
            }
        } else {
            m_streak = 0;
            m_discards.push_back(result.card);
            if(m_config.mode == MatchMode::Solo) {
                --m_lives;
                result.lifeLost = true;
            } else {
                // 対戦のルール：間違えたら1枚引く
                result.drawnCard = DrawCard();
                if(result.drawnCard) {
                    player.hand.push_back(*result.drawnCard);
                }
            }
        }
        result.streak = m_streak;

        if(m_config.mode == MatchMode::Solo) {
            if(m_lives <= 0) {
                m_outcome = MatchOutcome::SoloFailed;
            } else {
                // 手札を常に一定枚数に保つ
                if(static_cast<int>(player.hand.size()) < m_config.rules.soloHandSize) {
                    result.drawnCard = m_deck.Draw();
                    if(result.drawnCard) {
                        player.hand.push_back(*result.drawnCard);
                    }
                }
                if(player.hand.empty()) {
                    m_outcome = MatchOutcome::SoloCleared;
                }
            }
        }
        return result;
    }

    //----------------------------------------------------------------------------
    //! 手番を次のプレイヤーへ進めます。
    //----------------------------------------------------------------------------
    void Match::AdvanceTurn() {
        if(m_outcome != MatchOutcome::InProgress || m_config.mode == MatchMode::Solo) {
            return;
        }
        const size_t next = NextActivePlayer(m_current);
        // 番号が巻き戻ったら1周した＝ラウンドの終わり
        if(next <= m_current) {
            ResolveRoundEnd();
            if(m_outcome != MatchOutcome::InProgress) {
                return;
            }
            m_current = NextActivePlayer(m_players.size() - 1);
            return;
        }
        m_current = next;
    }

    //----------------------------------------------------------------------------
    //! 山札から1枚引きます。
    //----------------------------------------------------------------------------
    std::optional<CardData> Match::DrawCard() {
        if(m_deck.Remaining() == 0 && !m_discards.empty() && m_rng != nullptr) {
            // 長い対戦で山札が尽きたら、捨て札を混ぜ直して山札に戻す
            for(const CardData& card : m_discards) {
                m_deck.Add(card);
            }
            m_discards.clear();
            m_deck.Shuffle(*m_rng);
        }
        return m_deck.Draw();
    }

    //----------------------------------------------------------------------------
    //! ソロの得点を計算します。
    //----------------------------------------------------------------------------
    int Match::CalculateSoloScore(bool closeCall) const {
        // 連続正解ごとに 1 割ずつ倍率が上がる（上限あり）
        const RulesTuning& rules            = m_config.rules;
        const int          multiplierTenths = 10 + std::min(m_streak - 1, rules.streakCap);
        int                score            = rules.soloBaseScore * multiplierTenths / 10;
        if(closeCall) {
            score += rules.closeCallBonus;
        }
        return score;
    }

    //----------------------------------------------------------------------------
    //! ラウンドの終わりに勝敗を判定し、延長戦なら追加の手札を配ります。
    //----------------------------------------------------------------------------
    void Match::ResolveRoundEnd() {
        std::vector<size_t> finishers;
        for(size_t i = 0; i < m_players.size(); ++i) {
            if(!m_players[i].eliminated && m_players[i].hand.empty()) {
                finishers.push_back(i);
            }
        }

        ++m_round;
        if(finishers.empty()) {
            return;
        }
        if(finishers.size() == 1) {
            m_winner  = finishers.front();
            m_outcome = MatchOutcome::VersusWon;
            return;
        }

        // 同じラウンドに複数人が出し切った：その人たちだけで1枚ずつ追加して続ける
        m_tiebreak = true;
        for(size_t i = 0; i < m_players.size(); ++i) {
            const bool finished = std::find(finishers.begin(), finishers.end(), i) != finishers.end();
            if(!finished) {
                m_players[i].eliminated = true;
            }
        }
        for(size_t index : finishers) {
            if(const auto card = DrawCard()) {
                m_players[index].hand.push_back(*card);
            }
        }
    }

    //----------------------------------------------------------------------------
    //! 次に手番が回る、脱落していないプレイヤーを探します。
    //----------------------------------------------------------------------------
    size_t Match::NextActivePlayer(size_t from) const {
        const size_t count = m_players.size();
        for(size_t step = 1; step <= count; ++step) {
            const size_t index = (from + step) % count;
            if(!m_players[index].eliminated) {
                return index;
            }
        }
        return from;
    }

}    // namespace ChronoSort::Rules
