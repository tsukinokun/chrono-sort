//----------------------------------------------------------------------------
//! @file   CpuPlayer.cpp
//! @brief  CPU の手の選び方の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Rules/CpuPlayer.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>

namespace ChronoSort::Rules {

    namespace {

        //----------------------------------------------------------------------------
        //! 正しいスロットのうち、隣のカードとの年代差の最小値を返します。
        //! @param  [in] timeline 場の列。
        //! @param  [in] card     置くカード。
        //! @return 最小の年代差（年）。
        //----------------------------------------------------------------------------
        int NearestNeighborGap(const Timeline& timeline, const CardData& card) {
            const auto& cards = timeline.Cards();
            int         best  = 0;
            for(size_t slot : timeline.FindCorrectSlots(card)) {
                int gap = INT32_MAX;
                if(slot > 0) {
                    gap = std::min(gap, std::abs(card.year - cards[slot - 1].year));
                }
                if(slot < cards.size()) {
                    gap = std::min(gap, std::abs(card.year - cards[slot].year));
                }
                best = std::max(best, gap);
            }
            return best;
        }

    }    // namespace

    //----------------------------------------------------------------------------
    //! カードを正しい位置に置ける確率を返します。
    //----------------------------------------------------------------------------
    float CpuSuccessChance(CpuLevel level, const Timeline& timeline, const CardData& card, const RulesTuning& rules) {
        const CpuLevelRules& range = rules.Level(static_cast<size_t>(level));
        const float          ease  = std::clamp(static_cast<float>(NearestNeighborGap(timeline, card)) / rules.comfortableGapYears, 0.0f, 1.0f);
        return range.hardestChance + (range.easiestChance - range.hardestChance) * ease;
    }

    //----------------------------------------------------------------------------
    //! 手札から1枚選び、置くスロットを決めます。
    //----------------------------------------------------------------------------
    CpuMove ChooseCpuMove(const Player& player, const Timeline& timeline, std::mt19937& rng, const RulesTuning& rules) {
        CpuMove move;
        if(player.hand.empty()) {
            return move;
        }

        // 強いほど自信のあるカードから出す。弱い CPU は気まぐれに選ぶ
        std::uniform_real_distribution<float> unit(0.0f, 1.0f);
        const float                           whim = rules.Level(static_cast<size_t>(player.cpuLevel)).whim;
        if(unit(rng) < whim) {
            move.handIndex = std::uniform_int_distribution<size_t>(0, player.hand.size() - 1)(rng);
        } else {
            float best = -1.0f;
            for(size_t i = 0; i < player.hand.size(); ++i) {
                const float chance = CpuSuccessChance(player.cpuLevel, timeline, player.hand[i], rules);
                if(chance > best) {
                    best           = chance;
                    move.handIndex = i;
                }
            }
        }

        const CardData&           card     = player.hand[move.handIndex];
        const std::vector<size_t> correct  = timeline.FindCorrectSlots(card);
        const float               chance   = CpuSuccessChance(player.cpuLevel, timeline, card, rules);
        move.confidence                    = chance;
        const size_t              rightSlot = correct[std::uniform_int_distribution<size_t>(0, correct.size() - 1)(rng)];

        if(unit(rng) < chance || timeline.SlotCount() <= correct.size()) {
            move.slot = rightSlot;
            return move;
        }

        // 外すときは正解のすぐ隣（たまに2つ隣）を選ぶ。遠い場所に置くと不自然なため
        std::vector<size_t> nearSlots;
        std::vector<size_t> farSlots;
        for(int offset : {-1, 1, -2, 2}) {
            const long long candidate = static_cast<long long>(rightSlot) + offset;
            if(candidate < 0 || candidate >= static_cast<long long>(timeline.SlotCount())) {
                continue;
            }
            if(std::find(correct.begin(), correct.end(), static_cast<size_t>(candidate)) != correct.end()) {
                continue;
            }
            (std::abs(offset) == 1 ? nearSlots : farSlots).push_back(static_cast<size_t>(candidate));
        }
        const bool useNear = !nearSlots.empty() && (farSlots.empty() || unit(rng) < rules.nearMissChance);
        const std::vector<size_t>& pool = useNear ? nearSlots : farSlots;
        if(pool.empty()) {
            move.slot = rightSlot;
            return move;
        }
        move.slot = pool[std::uniform_int_distribution<size_t>(0, pool.size() - 1)(rng)];
        return move;
    }

}    // namespace ChronoSort::Rules
