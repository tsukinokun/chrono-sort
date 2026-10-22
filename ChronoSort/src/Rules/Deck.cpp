//----------------------------------------------------------------------------
//! @file   Deck.cpp
//! @brief  山札の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Rules/Deck.hpp>

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Path.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>

#include <algorithm>
#include <fstream>

namespace ChronoSort::Rules {

    //----------------------------------------------------------------------------
    //! 同梱のデッキ（日本史・世界史）をすべて読み込みます。
    //----------------------------------------------------------------------------
    std::vector<CardData> LoadAllCards() {
        std::vector<CardData> cards;
        LoadCardsFromFile("ChronoSort/Assets/Data/Decks/japan.json", cards);
        LoadCardsFromFile("ChronoSort/Assets/Data/Decks/world.json", cards);
        return cards;
    }

    //----------------------------------------------------------------------------
    //! デッキ JSON を読み込み、山札に追加します。
    //----------------------------------------------------------------------------
    bool Deck::LoadFromFile(const std::string& path) {
        return LoadCardsFromFile(path, m_cards);
    }

    //----------------------------------------------------------------------------
    //! デッキ JSON を読み込み、カード一覧の末尾に追加します。
    //----------------------------------------------------------------------------
    bool LoadCardsFromFile(const std::string& path, std::vector<CardData>& out) {
        const Tsukino::Core::Path filePath(path);

        std::ifstream is(filePath.string());
        if(!is) {
            Tsukino::Core::Log::Error("Deck file not found: " + filePath.string());
            return false;
        }

        std::string           deckName;
        std::vector<CardData> cards;
        try {
            cereal::JSONInputArchive archive(is);
            archive(cereal::make_nvp("deck", deckName), cereal::make_nvp("cards", cards));
        } catch(const std::exception& e) {
            // 書式ミスのデッキで起動ごと落ちないよう、読み込み失敗として扱う
            Tsukino::Core::Log::Error("Failed to parse deck file " + filePath.string() + ": " + e.what());
            return false;
        }

        out.insert(out.end(), cards.begin(), cards.end());
        Tsukino::Core::Log::Info("Loaded deck '" + deckName + "' (" + std::to_string(cards.size()) + " cards)");
        return true;
    }

    //----------------------------------------------------------------------------
    //! カードを1枚山札に追加します。
    //----------------------------------------------------------------------------
    void Deck::Add(const CardData& card) {
        m_cards.push_back(card);
    }

    //----------------------------------------------------------------------------
    //! 山札をシャッフルします。
    //----------------------------------------------------------------------------
    void Deck::Shuffle(std::mt19937& rng) {
        std::shuffle(m_cards.begin(), m_cards.end(), rng);
    }

    //----------------------------------------------------------------------------
    //! 山札の一番上から1枚引きます。
    //----------------------------------------------------------------------------
    std::optional<CardData> Deck::Draw() {
        if(m_cards.empty()) {
            return std::nullopt;
        }
        CardData card = std::move(m_cards.back());
        m_cards.pop_back();
        return card;
    }

}    // namespace ChronoSort::Rules
