//----------------------------------------------------------------------------
//! @file   RulesSelfTest.cpp
//! @brief  ルール判定の自己テストの実装
//----------------------------------------------------------------------------
#include <ChronoSort/Rules/RulesSelfTest.hpp>

#include <ChronoSort/Rules/CpuPlayer.hpp>
#include <ChronoSort/Rules/Deck.hpp>
#include <ChronoSort/Rules/Match.hpp>
#include <ChronoSort/Rules/RulesTuning.hpp>
#include <ChronoSort/Rules/Timeline.hpp>
#include <ChronoSort/Util/AssetPaths.hpp>

#include <Tsukino/Core/Log.hpp>

#include <algorithm>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace ChronoSort::Rules {

    namespace {

        //----------------------------------------------------------------------------
        //! 年だけを持つテスト用カードを作ります。
        //! @param  [in] id   識別子。
        //! @param  [in] year 西暦年。
        //! @return テスト用カード。
        //----------------------------------------------------------------------------
        CardData MakeCard(const char* id, int year) {
            CardData card;
            card.id   = id;
            card.year = year;
            return card;
        }

        //----------------------------------------------------------------------------
        //! 条件を確かめ、失敗したらログへ出します。
        //! @param  [in]     condition 期待どおりなら true。
        //! @param  [in]     name      テスト名。
        //! @param  [in,out] allPassed 全体の成否。失敗時に false にする。
        //----------------------------------------------------------------------------
        void Check(bool condition, const char* name, bool& allPassed) {
            if(!condition) {
                Tsukino::Core::Log::Error(std::string("[RulesSelfTest] FAILED: ") + name);
                allPassed = false;
            }
        }

        //----------------------------------------------------------------------------
        //! 年が 100 年ずつ離れたテスト用のカード一覧を作ります。
        //! @param  [in] count 枚数。
        //! @return カード一覧。
        //----------------------------------------------------------------------------
        std::vector<CardData> MakeCards(int count) {
            std::vector<CardData> cards;
            for(int i = 0; i < count; ++i) {
                cards.push_back(MakeCard("c", (i + 1) * 100));
                cards.back().id = "c" + std::to_string(i);
            }
            return cards;
        }

        //----------------------------------------------------------------------------
        //! 手番のプレイヤーの手札の先頭を、正しい（または間違った）位置に置きます。
        //! @param  [in,out] match   対局。
        //! @param  [in]     correct 正しい位置に置くなら true。
        //! @return 置いた結果。
        //----------------------------------------------------------------------------
        std::optional<PlayResult> PlayFirst(Match& match, bool correct) {
            const Player&   player = match.Players()[match.CurrentPlayerIndex()];
            const CardData& card   = player.hand.front();
            const auto      slots  = match.GetTimeline().FindCorrectSlots(card);
            size_t          slot   = slots.front();
            if(!correct) {
                // 正解以外のスロットを探す（年が全部違うので必ずある）
                for(size_t s = 0; s < match.GetTimeline().SlotCount(); ++s) {
                    if(!match.GetTimeline().IsCorrectSlot(s, card)) {
                        slot = s;
                        break;
                    }
                }
            }
            return match.Play(0, slot);
        }

        //----------------------------------------------------------------------------
        //! 対局の進行（ソロ・対戦・延長戦）を確かめます。
        //! @param  [in,out] passed 全体の成否。
        //----------------------------------------------------------------------------
        void TestMatch(bool& passed) {
            std::mt19937 rng(42);

            // ソロ
            {
                Match       solo;
                MatchConfig config;
                solo.Start(config, MakeCards(10), rng);
                Check(solo.Players().size() == 1 && solo.Players()[0].hand.size() == RulesTuning{}.soloHandSize, "solo deals hand", passed);
                Check(solo.GetTimeline().Cards().size() == 1 && solo.Lives() == RulesTuning{}.soloLives, "solo starts with one card and full lives", passed);

                const auto ok = PlayFirst(solo, true);
                Check(ok && ok->correct && ok->scoreGained > 0 && solo.Score() == ok->scoreGained, "solo correct play scores", passed);
                Check(solo.Players()[0].hand.size() == RulesTuning{}.soloHandSize, "solo refills hand after correct play", passed);

                const auto ng = PlayFirst(solo, false);
                Check(ng && !ng->correct && ng->lifeLost && solo.Lives() == RulesTuning{}.soloLives - 1, "solo wrong play loses a life", passed);
                Check(solo.Discards().size() == 1 && solo.Streak() == 0, "solo wrong play discards and resets streak", passed);

                PlayFirst(solo, false);
                PlayFirst(solo, false);
                Check(solo.Outcome() == MatchOutcome::SoloFailed, "solo fails when lives run out", passed);
                Check(!solo.Play(0, 0).has_value(), "no play after match end", passed);
            }

            // ソロのクリア（全問正解）
            {
                Match       solo;
                MatchConfig config;
                solo.Start(config, MakeCards(6), rng);
                int guard = 0;
                while(solo.Outcome() == MatchOutcome::InProgress && guard++ < 20) {
                    PlayFirst(solo, true);
                }
                Check(solo.Outcome() == MatchOutcome::SoloCleared && solo.BestStreak() == 5, "solo clears when all cards placed", passed);
            }

            // 対戦：2人とも全問正解すると同じラウンドで出し切り、延長戦になる
            {
                Match       versus;
                MatchConfig config;
                config.mode = MatchMode::Versus;
                config.cpus = {CpuLevel::Normal};
                versus.Start(config, MakeCards(30), rng);
                Check(versus.Players().size() == 2 && versus.Players()[1].hand.size() == static_cast<size_t>(RulesTuning{}.InitialHandSize(2)), "versus deals by player count", passed);

                for(int turn = 0; turn < RulesTuning{}.InitialHandSize(2) * 2; ++turn) {
                    PlayFirst(versus, true);
                    versus.AdvanceTurn();
                }
                Check(versus.IsTiebreak() && versus.Outcome() == MatchOutcome::InProgress, "simultaneous finish starts tiebreak", passed);
                Check(versus.Players()[0].hand.size() == 1 && versus.Players()[1].hand.size() == 1, "tiebreak deals one card each", passed);

                // 延長戦：人間は正解、CPU は不正解 → 人間の勝ち
                PlayFirst(versus, true);
                versus.AdvanceTurn();
                const auto cpuMiss = PlayFirst(versus, false);
                Check(cpuMiss && cpuMiss->drawnCard.has_value(), "versus wrong play draws a card", passed);
                versus.AdvanceTurn();
                Check(versus.Outcome() == MatchOutcome::VersusWon && versus.Winner() == 0, "tiebreak winner decided", passed);
            }

            // 対戦：脱落者は手番が飛ばされる
            {
                Match       versus;
                MatchConfig config;
                config.mode = MatchMode::Versus;
                config.cpus = {CpuLevel::Easy, CpuLevel::Easy};
                versus.Start(config, MakeCards(40), rng);
                // 人間と CPU1 が出し切り、CPU2 は間違え続ける
                for(int turn = 0; turn < RulesTuning{}.InitialHandSize(3) * 3; ++turn) {
                    PlayFirst(versus, versus.CurrentPlayerIndex() != 2);
                    versus.AdvanceTurn();
                }
                Check(versus.IsTiebreak() && versus.Players()[2].eliminated, "non-finisher is eliminated in tiebreak", passed);
                Check(versus.CurrentPlayerIndex() == 0, "turn returns to first active player", passed);
                PlayFirst(versus, true);
                versus.AdvanceTurn();
                Check(versus.CurrentPlayerIndex() == 1, "eliminated player is skipped", passed);
            }
        }

        //----------------------------------------------------------------------------
        //! CPU の手の選び方を確かめます。
        //! @param  [in,out] passed 全体の成否。
        //----------------------------------------------------------------------------
        void TestCpu(bool& passed) {
            std::mt19937 rng(7);
            Timeline     timeline;
            timeline.Reset(MakeCard("a", 1000));
            timeline.TryPlace(1, MakeCard("b", 1010));

            const CardData closeCard = MakeCard("closeCard", 1005);
            const CardData farCard   = MakeCard("farCard", 1800);
            Check(CpuSuccessChance(CpuLevel::Hard, timeline, closeCard, RulesTuning{}) > CpuSuccessChance(CpuLevel::Easy, timeline, closeCard, RulesTuning{}), "hard cpu beats easy cpu", passed);
            Check(CpuSuccessChance(CpuLevel::Normal, timeline, farCard, RulesTuning{}) > CpuSuccessChance(CpuLevel::Normal, timeline, closeCard, RulesTuning{}), "farCard years are easier", passed);

            Player cpu;
            cpu.isCpu    = true;
            cpu.cpuLevel = CpuLevel::Easy;
            cpu.hand     = {closeCard, farCard};
            int correct  = 0;
            for(int i = 0; i < 200; ++i) {
                const CpuMove move = ChooseCpuMove(cpu, timeline, rng, RulesTuning{});
                Check(move.handIndex < cpu.hand.size() && move.slot < timeline.SlotCount(), "cpu move in range", passed);
                correct += timeline.IsCorrectSlot(move.slot, cpu.hand[move.handIndex]) ? 1 : 0;
            }
            // 弱い CPU でも当てずっぽうよりは当たり、全問正解はしない
            Check(correct > 60 && correct < 200, "easy cpu is fallible but not random", passed);
        }

    }    // namespace

    //----------------------------------------------------------------------------
    //! ルール判定の境界ケースを実行し、結果をログへ出します。
    //----------------------------------------------------------------------------
    bool RunSelfTest() {
        bool passed = true;

        // 場：[843 ヴェルダン条約] から始める
        Timeline timeline;
        timeline.Reset(MakeCard("verdun", 843));

        // 左端・右端
        Check(timeline.IsCorrectSlot(0, MakeCard("before", 600)), "left end accepts earlier card", passed);
        Check(!timeline.IsCorrectSlot(1, MakeCard("before", 600)), "right end rejects earlier card", passed);
        Check(timeline.TryPlace(1, MakeCard("taisei", 1867)), "place later card at right end", passed);

        // 場：[843, 1867]。間の隙間
        Check(timeline.IsCorrectSlot(1, MakeCard("mid", 1600)), "middle slot accepts card in range", passed);
        Check(!timeline.IsCorrectSlot(0, MakeCard("mid", 1600)), "left end rejects card in range", passed);
        Check(!timeline.IsCorrectSlot(2, MakeCard("mid", 1600)), "right end rejects card in range", passed);

        // 同じ年は左右どちらでも正解
        Check(timeline.IsCorrectSlot(0, MakeCard("same", 843)), "same year left side", passed);
        Check(timeline.IsCorrectSlot(1, MakeCard("same", 843)), "same year right side", passed);
        Check(timeline.FindCorrectSlots(MakeCard("same", 843)).size() == 2, "same year has two correct slots", passed);

        // 紀元前（負の年）
        Check(timeline.TryPlace(0, MakeCard("qin", -221)), "BC card at left end", passed);
        Check(!timeline.IsCorrectSlot(1, MakeCard("rome", -753)), "older BC card rejected after newer BC card", passed);
        Check(timeline.IsCorrectSlot(0, MakeCard("rome", -753)), "older BC card accepted before newer BC card", passed);

        // 間違いでは列が変わらないこと
        const size_t countBefore = timeline.Cards().size();
        Check(!timeline.TryPlace(3, MakeCard("wrong", 1000)), "wrong placement is rejected", passed);
        Check(timeline.Cards().size() == countBefore, "wrong placement keeps timeline unchanged", passed);

        // 範囲外のスロット
        Check(!timeline.IsCorrectSlot(timeline.SlotCount(), MakeCard("any", 2000)), "out of range slot is rejected", passed);

        // 人数ごとの手札枚数
        Check(RulesTuning{}.InitialHandSize(2) == 6 && RulesTuning{}.InitialHandSize(3) == 6, "hand size for 2-3 players", passed);
        Check(RulesTuning{}.InitialHandSize(4) == 5 && RulesTuning{}.InitialHandSize(5) == 5, "hand size for 4-5 players", passed);
        Check(RulesTuning{}.InitialHandSize(6) == 4 && RulesTuning{}.InitialHandSize(8) == 4, "hand size for 6-8 players", passed);

        // 山札
        Deck deck;
        deck.Add(MakeCard("a", 1));
        deck.Add(MakeCard("b", 2));
        Check(deck.Draw().has_value() && deck.Remaining() == 1, "deck draw reduces count", passed);
        deck.Draw();
        Check(!deck.Draw().has_value(), "empty deck returns nullopt", passed);

        // 同梱デッキが読めること
        Deck japan;
        Deck world;
        Check(japan.LoadFromFile(Util::AssetPaths::Deck("japan")) && japan.Remaining() > 0, "load japan deck", passed);
        Check(world.LoadFromFile(Util::AssetPaths::Deck("world")) && world.Remaining() > 0, "load world deck", passed);

        // 出題の範囲で絞る
        const auto japanCards = LoadCards(CardRange::Japan);
        const auto worldCards = LoadCards(CardRange::World);
        Check(!japanCards.empty() && std::all_of(japanCards.begin(), japanCards.end(), [](const CardData& c) { return c.region == "JP"; }),
              "japan range has only JP cards", passed);
        Check(!worldCards.empty() && std::all_of(worldCards.begin(), worldCards.end(), [](const CardData& c) { return c.region == "WORLD"; }),
              "world range has only WORLD cards", passed);
        Check(japanCards.size() + worldCards.size() == LoadCards(CardRange::All).size(), "ranges split all cards", passed);
        Check(RangeFromKey(RangeKey(CardRange::World)) == CardRange::World && RangeFromKey("unknown") == CardRange::All, "range key round trip", passed);

        TestMatch(passed);
        TestCpu(passed);

        Tsukino::Core::Log::Info(passed ? "[RulesSelfTest] All tests passed." : "[RulesSelfTest] Some tests FAILED.");
        return passed;
    }

}    // namespace ChronoSort::Rules
