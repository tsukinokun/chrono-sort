//----------------------------------------------------------------------------
//! @file   RulesSelfTest.cpp
//! @brief  ルール判定の自己テストの実装
//----------------------------------------------------------------------------
#include <ChronoSort/Rules/RulesSelfTest.hpp>

#include <ChronoSort/Rules/Deck.hpp>
#include <ChronoSort/Rules/GameRules.hpp>
#include <ChronoSort/Rules/Timeline.hpp>

#include <Tsukino/Core/Log.hpp>

#include <string>

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
        Check(GetInitialHandSize(2) == 6 && GetInitialHandSize(3) == 6, "hand size for 2-3 players", passed);
        Check(GetInitialHandSize(4) == 5 && GetInitialHandSize(5) == 5, "hand size for 4-5 players", passed);
        Check(GetInitialHandSize(6) == 4 && GetInitialHandSize(8) == 4, "hand size for 6-8 players", passed);

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
        Check(japan.LoadFromFile("ChronoSort/Assets/Data/Decks/japan.json") && japan.Remaining() > 0, "load japan deck", passed);
        Check(world.LoadFromFile("ChronoSort/Assets/Data/Decks/world.json") && world.Remaining() > 0, "load world deck", passed);

        Tsukino::Core::Log::Info(passed ? "[RulesSelfTest] All tests passed." : "[RulesSelfTest] Some tests FAILED.");
        return passed;
    }

}    // namespace ChronoSort::Rules
