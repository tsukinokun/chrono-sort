//----------------------------------------------------------------------------
//! @file   Timeline.cpp
//! @brief  場に並んだカードの列の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Rules/Timeline.hpp>

namespace ChronoSort::Rules {

    //----------------------------------------------------------------------------
    //! 場の最初の1枚を置きます。既に置かれているカードは消えます。
    //----------------------------------------------------------------------------
    void Timeline::Reset(const CardData& card) {
        m_cards.clear();
        m_cards.push_back(card);
    }

    //----------------------------------------------------------------------------
    //! 指定スロットに置いたとき、年代順が保たれるかを判定します。
    //----------------------------------------------------------------------------
    bool Timeline::IsCorrectSlot(size_t slot, const CardData& card) const {
        if(slot >= SlotCount()) {
            return false;
        }

        // 左隣より前ではなく、右隣より後ではないこと。
        // 等号を含めることで、同じ年は左右どちらに置いても正解になる
        const bool afterLeft   = (slot == 0) || (m_cards[slot - 1].year <= card.year);
        const bool beforeRight = (slot == m_cards.size()) || (card.year <= m_cards[slot].year);
        return afterLeft && beforeRight;
    }

    //----------------------------------------------------------------------------
    //! 正しければ指定スロットにカードを挿入します。
    //----------------------------------------------------------------------------
    bool Timeline::TryPlace(size_t slot, const CardData& card) {
        if(!IsCorrectSlot(slot, card)) {
            return false;
        }
        m_cards.insert(m_cards.begin() + static_cast<std::ptrdiff_t>(slot), card);
        return true;
    }

    //----------------------------------------------------------------------------
    //! カードを正しく置けるスロットをすべて返します。
    //----------------------------------------------------------------------------
    std::vector<size_t> Timeline::FindCorrectSlots(const CardData& card) const {
        std::vector<size_t> slots;
        for(size_t slot = 0; slot < SlotCount(); ++slot) {
            if(IsCorrectSlot(slot, card)) {
                slots.push_back(slot);
            }
        }
        return slots;
    }

}    // namespace ChronoSort::Rules
