//----------------------------------------------------------------------------
//! @file   Timeline.hpp
//! @brief  場に並んだカードの列
//! @detail 年代順に並んだカード列と、隙間（スロット）へ置けるかの判定を持ちます。
//!         スロット番号は「i 番目のカードの左」を i とし、右端はカード枚数と同じ値です。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/CardData.hpp>

#include <vector>

namespace ChronoSort::Rules {

    //! 場に並んだカードの列です。
    class Timeline {
    public:

        //! 場の最初の1枚を置きます。既に置かれているカードは消えます。
        //! @param  [in] card 最初のカード。
        void Reset(const CardData& card);

        //! 指定スロットに置いたとき、年代順が保たれるかを判定します。
        //! 同じ年のカードは左右どちらに置いても正解として扱います。
        //! @param  [in] slot 置くスロット（0 〜 SlotCount()-1）。
        //! @param  [in] card 置くカード。
        //! @return 正しい位置なら true。範囲外のスロットは false。
        bool IsCorrectSlot(size_t slot, const CardData& card) const;

        //! 正しければ指定スロットにカードを挿入します。
        //! @param  [in] slot 置くスロット。
        //! @param  [in] card 置くカード。
        //! @return 挿入したら true。間違いなら列は変えずに false。
        bool TryPlace(size_t slot, const CardData& card);

        //! カードを正しく置けるスロットをすべて返します。
        //! 同じ年のカードが並んでいると候補は複数になります。
        //! @param  [in] card 調べるカード。
        //! @return 正しいスロット番号の一覧（昇順）。
        std::vector<size_t> FindCorrectSlots(const CardData& card) const;

        //! 場のカード列を返します。
        //! @return 年代順のカード列。
        const std::vector<CardData>& Cards() const { return m_cards; }

        //! スロットの数（カード枚数 + 1）を返します。
        //! @return スロット数。
        size_t SlotCount() const { return m_cards.size() + 1; }

    private:
        std::vector<CardData> m_cards;    // 年代順に並んだ場のカード
    };

}    // namespace ChronoSort::Rules
