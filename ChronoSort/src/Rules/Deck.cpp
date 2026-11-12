//----------------------------------------------------------------------------
//! @file   Deck.cpp
//! @brief  山札の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Rules/Deck.hpp>

#include <ChronoSort/Util/AssetPaths.hpp>
#include <ChronoSort/Util/Localization.hpp>
#include <ChronoSort/Util/TextUtil.hpp>

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Path.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>

#include <algorithm>
#include <fstream>

namespace ChronoSort::Rules {

    namespace {
        //----------------------------------------------------------------------------
        //! カードの文言を今の言語に差し替えます（日本語はデッキ JSON が原文なのでそのまま）。
        //! @param  [in,out] card カード。
        //----------------------------------------------------------------------------
        void LocalizeCard(CardData& card) {
            if(Util::CurrentLanguage() == Util::Language::Japanese) {
                return;
            }
            if(const Util::CardText* text = Util::FindCardText(card.id)) {
                if(!text->title.empty()) {
                    card.title = text->title;
                }
                if(!text->caption.empty()) {
                    card.caption = text->caption;
                }
            }
            // 分類は11種しかないので、文字列の表で引く。年号は西暦の値から作る
            card.category  = Util::WideToUtf8(Util::Text("category." + card.category));
            card.yearLabel = Util::WideToUtf8(card.year < 0 ? Util::Format("card.yearBC", {std::to_wstring(-card.year)})
                                                            : Util::Format("card.year", {std::to_wstring(card.year)}));
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! 同梱のデッキ（日本史・世界史）をすべて読み込みます。
    //----------------------------------------------------------------------------
    std::vector<CardData> LoadAllCards() {
        std::vector<CardData> cards;
        LoadCardsFromFile(Util::AssetPaths::Deck("japan"), cards);
        LoadCardsFromFile(Util::AssetPaths::Deck("world"), cards);
        return cards;
    }

    //----------------------------------------------------------------------------
    //! 範囲と難易度に入るカードだけを読み込みます。
    //----------------------------------------------------------------------------
    std::vector<CardData> LoadCards(CardRange range, CardDifficulty difficulty) {
        std::vector<CardData> cards = LoadAllCards();
        const int             limit = static_cast<int>(difficulty);
        std::erase_if(cards, [limit](const CardData& card) { return card.difficulty > limit; });
        if(range == CardRange::All) {
            return cards;
        }
        const std::string region = (range == CardRange::Japan) ? "JP" : "WORLD";
        std::erase_if(cards, [&](const CardData& card) { return card.region != region; });
        return cards;
    }

    //----------------------------------------------------------------------------
    //! 範囲を保存用のキーに変えます。
    //----------------------------------------------------------------------------
    const char* RangeKey(CardRange range) {
        switch(range) {
        case CardRange::Japan:
            return "japan";
        case CardRange::World:
            return "world";
        default:
            return "all";
        }
    }

    //----------------------------------------------------------------------------
    //! 範囲の表示名を返します。
    //----------------------------------------------------------------------------
    std::wstring RangeName(CardRange range) {
        return Util::Text(std::string("range.") + RangeKey(range));
    }

    //----------------------------------------------------------------------------
    //! 保存用のキーから範囲を返します。
    //----------------------------------------------------------------------------
    CardRange RangeFromKey(const std::string& key) {
        if(key == "japan") {
            return CardRange::Japan;
        }
        if(key == "world") {
            return CardRange::World;
        }
        return CardRange::All;
    }

    //----------------------------------------------------------------------------
    //! 難易度を保存用のキーに変えます。
    //----------------------------------------------------------------------------
    const char* DifficultyKey(CardDifficulty difficulty) {
        switch(difficulty) {
        case CardDifficulty::Easy:
            return "easy";
        case CardDifficulty::Normal:
            return "normal";
        default:
            return "hard";
        }
    }

    //----------------------------------------------------------------------------
    //! 難易度の表示名を返します。
    //----------------------------------------------------------------------------
    std::wstring DifficultyName(CardDifficulty difficulty) {
        return Util::Text(std::string("difficulty.") + DifficultyKey(difficulty));
    }

    //----------------------------------------------------------------------------
    //! 保存用のキーから難易度を返します。
    //----------------------------------------------------------------------------
    CardDifficulty DifficultyFromKey(const std::string& key) {
        if(key == "easy") {
            return CardDifficulty::Easy;
        }
        if(key == "normal") {
            return CardDifficulty::Normal;
        }
        return CardDifficulty::Hard;
    }

    //----------------------------------------------------------------------------
    //! 出題の範囲と難易度の表示名を返します。
    //----------------------------------------------------------------------------
    std::wstring ProblemSetName(CardRange range, CardDifficulty difficulty) {
        // むずかしいは全部のカードなので、難易度を分ける前と同じく範囲の名前だけにする
        if(difficulty == CardDifficulty::Hard) {
            return RangeName(range);
        }
        return Util::Format("problemSet", {RangeName(range), DifficultyName(difficulty)});
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

        for(CardData& card : cards) {
            LocalizeCard(card);
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
