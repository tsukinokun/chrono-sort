//----------------------------------------------------------------------------
//! @file   Deck.hpp
//! @brief  山札
//! @detail デッキ JSON の読み込み・シャッフル・ドローを担当します。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/CardData.hpp>

#include <optional>
#include <random>
#include <string>
#include <vector>

namespace ChronoSort::Rules {

    //! デッキ JSON を読み込み、カード一覧の末尾に追加します。
    //! @param  [in]     path  ワークスペースルートからの相対パス。
    //! @param  [in,out] cards 追加先のカード一覧。
    //! @return 読み込みに成功したら true。
    bool LoadCardsFromFile(const std::string& path, std::vector<CardData>& cards);

    //! 同梱のデッキ（日本史・世界史）をすべて読み込みます。
    //! @return 全カード。
    std::vector<CardData> LoadAllCards();

    //! 出題の範囲です。
    enum class CardRange {
        All,      // 日本史と世界史（通史）
        Japan,    // 日本史だけ
        World,    // 世界史だけ
    };

    //! 範囲に入るカードだけを読み込みます。
    //! @param  [in] range 出題の範囲。
    //! @return 範囲に入るカード。
    std::vector<CardData> LoadCards(CardRange range);

    //! 範囲を保存用のキー（"all" / "japan" / "world"）に変えます。
    //! @param  [in] range 出題の範囲。
    //! @return キー。
    const char* RangeKey(CardRange range);

    //! 範囲の表示名（UTF-8。「日本史」「世界史」「通史」）を返します。
    //! @param  [in] range 出題の範囲。
    //! @return 表示名。
    const char* RangeName(CardRange range);

    //! 保存用のキーから範囲を返します。知らないキーは通史として扱います。
    //! @param  [in] key キー。
    //! @return 出題の範囲。
    CardRange RangeFromKey(const std::string& key);

    //! 山札です。末尾を山の一番上として扱います。
    class Deck {
    public:

        //! デッキ JSON を読み込み、山札に追加します。
        //! @param  [in] path ワークスペースルートからの相対パス。
        //! @return 読み込みに成功したら true。
        bool LoadFromFile(const std::string& path);

        //! カードを1枚山札に追加します。
        //! @param  [in] card 追加するカード。
        void Add(const CardData& card);

        //! 山札をシャッフルします。
        //! @param  [in,out] rng 乱数生成器。
        void Shuffle(std::mt19937& rng);

        //! 山札の一番上から1枚引きます。
        //! @return 引いたカード。山札が空なら std::nullopt。
        std::optional<CardData> Draw();

        //! 山札の残り枚数を返します。
        //! @return 残り枚数。
        size_t Remaining() const { return m_cards.size(); }

    private:
        std::vector<CardData> m_cards;    // 山札（末尾が一番上）
    };

}    // namespace ChronoSort::Rules
