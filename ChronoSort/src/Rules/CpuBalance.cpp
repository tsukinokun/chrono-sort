//----------------------------------------------------------------------------
//! @file   CpuBalance.cpp
//! @brief  対戦 CPU の強さの測定の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Rules/CpuBalance.hpp>

#include <ChronoSort/Rules/CpuPlayer.hpp>
#include <ChronoSort/Rules/Match.hpp>

#include <Tsukino/Core/Log.hpp>

#include <cstdio>
#include <random>

namespace ChronoSort::Rules {

    namespace {

        constexpr int kMatchesPerCase = 2000;    // 1通りあたりの対戦の数
        constexpr int kTurnLimit      = 400;     // 決着しない対戦を打ち切る手数（山札が尽きて回り続けるのを防ぐ）

        //! 人の代わりが置くスロットを決めます。
        //! @param  [in]     timeline 場の列。
        //! @param  [in]     card     置くカード。
        //! @param  [in]     accuracy 正しい場所に置く確率。
        //! @param  [in,out] rng      乱数生成器。
        //! @return 置くスロット。
        size_t ChooseStandInSlot(const Timeline& timeline, const CardData& card, float accuracy, std::mt19937& rng) {
            const std::vector<size_t> correct = timeline.FindCorrectSlots(card);
            const size_t              right   = correct[std::uniform_int_distribution<size_t>(0, correct.size() - 1)(rng)];
            if(std::uniform_real_distribution<float>(0.0f, 1.0f)(rng) < accuracy) {
                return right;
            }
            // 外すときは CPU と同じく、正解のすぐ隣のうち正しくない方へ置く
            for(int offset : {-1, 1}) {
                const long long candidate = static_cast<long long>(right) + offset;
                if(candidate >= 0 && candidate < static_cast<long long>(timeline.SlotCount()) && !timeline.IsCorrectSlot(static_cast<size_t>(candidate), card)) {
                    return static_cast<size_t>(candidate);
                }
            }
            return right;
        }

        //! 1通り（CPU の段と、人の代わりの正解率）の結果です。
        struct CaseResult {
            int   standInWins = 0;       // 人の代わりが勝った数
            int   decided     = 0;       // 決着した数
            int   cpuPlays    = 0;       // CPU が置いた数
            int   cpuCorrect  = 0;       // CPU が正しく置いた数
            long  turns       = 0;       // 決着までの手数の合計（決着した対戦だけ）
        };

        //! 1通りぶんの対戦を回します。
        CaseResult RunCase(const RulesTuning& rules, const std::vector<CardData>& cards, CpuLevel level, float accuracy, std::mt19937& rng) {
            CaseResult result;
            for(int m = 0; m < kMatchesPerCase; ++m) {
                MatchConfig config;
                config.mode  = MatchMode::Versus;
                config.cpus  = {level};
                config.rules = rules;
                Match match;
                match.Start(config, cards, rng);

                int turn = 0;
                for(; turn < kTurnLimit && match.Outcome() == MatchOutcome::InProgress; ++turn) {
                    const Player& player = match.Players()[match.CurrentPlayerIndex()];
                    if(player.hand.empty()) {
                        match.AdvanceTurn();
                        continue;
                    }
                    size_t handIndex = 0;
                    size_t slot      = 0;
                    if(player.isCpu) {
                        const CpuMove move = ChooseCpuMove(player, match.GetTimeline(), rng, rules);
                        handIndex          = move.handIndex;
                        slot               = move.slot;
                    } else {
                        handIndex = std::uniform_int_distribution<size_t>(0, player.hand.size() - 1)(rng);
                        slot      = ChooseStandInSlot(match.GetTimeline(), player.hand[handIndex], accuracy, rng);
                    }
                    const bool cpu    = player.isCpu;
                    const auto played = match.Play(handIndex, slot);
                    if(cpu && played) {
                        ++result.cpuPlays;
                        result.cpuCorrect += played->correct ? 1 : 0;
                    }
                    match.AdvanceTurn();
                }
                if(match.Outcome() == MatchOutcome::VersusWon) {
                    ++result.decided;
                    result.turns += turn;
                    result.standInWins += (match.Winner() == 0) ? 1 : 0;
                }
            }
            return result;
        }

    }    // namespace

    //----------------------------------------------------------------------------
    //! CPU の段ごとに、人の代わりの打ち手との対戦を回して結果をログへ出します。
    //----------------------------------------------------------------------------
    void MeasureCpuBalance(const RulesTuning& rules, const std::vector<CardData>& cards) {
        const char* const kLevelNames[] = {"easy", "normal", "hard"};
        std::mt19937      rng(20261104);
        for(int level = 0; level < 3; ++level) {
            for(float accuracy : {0.65f, 0.72f, 0.80f}) {
                const CaseResult r = RunCase(rules, cards, static_cast<CpuLevel>(level), accuracy, rng);
                char             line[200];
                std::snprintf(line, sizeof(line), "[CpuBalance] %-6s vs stand-in %.2f: stand-in wins %5.1f%%, cpu accuracy %.2f, turns %.1f (%d/%d decided)",
                              kLevelNames[level], accuracy, r.decided > 0 ? 100.0f * r.standInWins / r.decided : 0.0f,
                              r.cpuPlays > 0 ? static_cast<float>(r.cpuCorrect) / r.cpuPlays : 0.0f, r.decided > 0 ? static_cast<float>(r.turns) / r.decided : 0.0f,
                              r.decided, kMatchesPerCase);
                Tsukino::Core::Log::Info(line);
            }
        }
    }

}    // namespace ChronoSort::Rules
