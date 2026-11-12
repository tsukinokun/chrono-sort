//----------------------------------------------------------------------------
//! @file   Localization.cpp
//! @brief  表示言語と文字列の表の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Util/Localization.hpp>

#include <ChronoSort/Util/AssetPaths.hpp>
#include <ChronoSort/Util/TextUtil.hpp>

#include <Tsukino/Core/Log.hpp>

#include <cereal/external/rapidjson/document.h>

#include <fstream>
#include <set>
#include <sstream>
#include <unordered_map>

namespace ChronoSort::Util {

    namespace {
        namespace Json = CEREAL_RAPIDJSON_NAMESPACE;

        //! 読み込んだ表です。
        struct Tables {
            Language                                      current = Language::Japanese;    // 今の言語
            std::unordered_map<std::string, std::wstring> text;                            // 今の言語の文字列
            std::unordered_map<std::string, std::wstring> fallback;                        // 日本語の文字列（足りないキーの代わり）
            std::unordered_map<std::string, CardText>     cards;                           // 今の言語のカードの訳
            std::unordered_map<std::string, std::wstring> missing;                         // 表に無かったキー（キーそのものを返すために持つ）
        };

        //----------------------------------------------------------------------------
        //! 表を返します。
        //! @return 表。
        //----------------------------------------------------------------------------
        Tables& GetTables() {
            static Tables tables;
            return tables;
        }

        //----------------------------------------------------------------------------
        //! JSON ファイルを読み込みます。
        //! @param  [in]  path     パス。
        //! @param  [out] document 読んだ文書。
        //! @return 読めたら true。
        //----------------------------------------------------------------------------
        bool ReadJson(const std::string& path, Json::Document& document) {
            std::ifstream is(path, std::ios::binary);
            if(!is) {
                Tsukino::Core::Log::Warn("Text table not found: " + path);
                return false;
            }
            std::stringstream buffer;
            buffer << is.rdbuf();
            const std::string text = buffer.str();
            document.Parse(text.c_str(), text.size());
            if(document.HasParseError() || !document.IsObject()) {
                Tsukino::Core::Log::Warn("Failed to parse text table: " + path);
                return false;
            }
            return true;
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! 言語を保存用のコードに変えます。
    //----------------------------------------------------------------------------
    const char* LanguageCode(Language language) {
        return language == Language::English ? "en" : "ja";
    }

    //----------------------------------------------------------------------------
    //! 保存用のコードから言語を返します。
    //----------------------------------------------------------------------------
    Language LanguageFromCode(const std::string& code) {
        return code == "en" ? Language::English : Language::Japanese;
    }

    //----------------------------------------------------------------------------
    //! 言語の表をそのまま読みます。
    //----------------------------------------------------------------------------
    std::map<std::string, std::string> ReadTextTable(Language language) {
        std::map<std::string, std::string> table;
        Json::Document                     document;
        if(!ReadJson(AssetPaths::TextTable(LanguageCode(language)), document)) {
            return table;
        }
        for(auto it = document.MemberBegin(); it != document.MemberEnd(); ++it) {
            if(it->value.IsString()) {
                table.emplace(it->name.GetString(), it->value.GetString());
            }
        }
        return table;
    }

    //----------------------------------------------------------------------------
    //! 言語のカードの表をそのまま読みます。
    //----------------------------------------------------------------------------
    std::map<std::string, CardText> ReadCardTable(Language language) {
        std::map<std::string, CardText> table;
        // 日本語はデッキ JSON が原文なので、カードの表は無い
        if(language == Language::Japanese) {
            return table;
        }
        Json::Document document;
        if(!ReadJson(AssetPaths::CardTextTable(LanguageCode(language)), document)) {
            return table;
        }
        for(auto it = document.MemberBegin(); it != document.MemberEnd(); ++it) {
            if(!it->value.IsObject()) {
                continue;
            }
            CardText text;
            if(auto title = it->value.FindMember("title"); title != it->value.MemberEnd() && title->value.IsString()) {
                text.title = title->value.GetString();
            }
            if(auto caption = it->value.FindMember("caption"); caption != it->value.MemberEnd() && caption->value.IsString()) {
                text.caption = caption->value.GetString();
            }
            table.emplace(it->name.GetString(), std::move(text));
        }
        return table;
    }

    //----------------------------------------------------------------------------
    //! 言語の表を読み込み、今の言語にします。
    //----------------------------------------------------------------------------
    void LoadLanguage(Language language) {
        Tables& tables = GetTables();
        tables.current = language;
        tables.text.clear();
        tables.fallback.clear();
        tables.cards.clear();
        tables.missing.clear();

        for(const auto& [key, value] : ReadTextTable(Language::Japanese)) {
            tables.fallback.emplace(key, Utf8ToWide(value));
        }
        if(language != Language::Japanese) {
            for(const auto& [key, value] : ReadTextTable(language)) {
                tables.text.emplace(key, Utf8ToWide(value));
            }
            for(auto& [id, text] : ReadCardTable(language)) {
                tables.cards.emplace(id, std::move(text));
            }
        }
        Tsukino::Core::Log::Info(std::string("Language: ") + LanguageCode(language));
    }

    //----------------------------------------------------------------------------
    //! 今の言語を返します。
    //----------------------------------------------------------------------------
    Language CurrentLanguage() {
        return GetTables().current;
    }

    //----------------------------------------------------------------------------
    //! キーの文字列を今の言語で返します。
    //----------------------------------------------------------------------------
    const std::wstring& Text(std::string_view key) {
        Tables&           tables = GetTables();
        const std::string name(key);
        if(auto it = tables.text.find(name); it != tables.text.end()) {
            return it->second;
        }
        if(auto it = tables.fallback.find(name); it != tables.fallback.end()) {
            return it->second;
        }
        // 表に無いキーは、気づけるようにキーそのものを出す（警告は1回だけ）
        auto [it, inserted] = tables.missing.emplace(name, Utf8ToWide(name));
        if(inserted) {
            Tsukino::Core::Log::Warn("[Text] missing key: " + name);
        }
        return it->second;
    }

    //----------------------------------------------------------------------------
    //! キーの文字列の {0} {1} … を置き換えて返します。
    //----------------------------------------------------------------------------
    std::wstring Format(std::string_view key, std::initializer_list<std::wstring> args) {
        std::wstring text = Text(key);
        size_t       index = 0;
        for(const std::wstring& arg : args) {
            const std::wstring mark = L"{" + std::to_wstring(index++) + L"}";
            for(size_t at = text.find(mark); at != std::wstring::npos; at = text.find(mark, at + arg.size())) {
                text.replace(at, mark.size(), arg);
            }
        }
        return text;
    }

    //----------------------------------------------------------------------------
    //! 今の言語でのカードの訳を返します。
    //----------------------------------------------------------------------------
    const CardText* FindCardText(const std::string& id) {
        const Tables& tables = GetTables();
        auto          it     = tables.cards.find(id);
        return it != tables.cards.end() ? &it->second : nullptr;
    }

}    // namespace ChronoSort::Util
