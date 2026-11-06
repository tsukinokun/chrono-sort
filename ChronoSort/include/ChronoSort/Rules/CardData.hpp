//----------------------------------------------------------------------------
//! @file   CardData.hpp
//! @brief  歴史カード1枚分のデータ
//! @detail デッキ JSON から読み込む値そのものです。表示や ECS には依存しません。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/IO/CerealOptional.hpp>

#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>

#include <cstdint>
#include <string>

namespace ChronoSort::Rules {

    constexpr size_t kCaptionCharsPerLine = 28;    // 説明文の1行の字数（全角）
    constexpr size_t kCaptionMaxLines     = 3;     // 説明文の行数の上限

    //! 歴史カード1枚分のデータです。
    struct CardData {
        std::string  id;           // 一意な識別子（画像ファイル名にも使う）
        std::string  title;        // 出来事の名前（UTF-8）
        std::int32_t year = 0;     // 西暦年。紀元前は負の数。並び順の判定はこの値だけで行う
        std::string  yearLabel;    // 画面に出す年号表記（例：「紀元前221年」）
        std::string  region;       // 地域（"JP" / "WORLD"）
        std::string  category;     // 分類（政治・文化など）
        std::string  art;          // カード絵の出典（URL など）。焼き込み後の面は id から決まる
        int          difficulty = 3;    // 難易度（1: やさしい, 2: ふつう, 3: むずかしい）。出題の難易度以下のカードだけが出る
        std::string  caption;      // 出来事の説明（対局の後に、カードを拡大した紙へ書く）
        std::string  wiki;         // Wikipedia（日本語版）の記事名。空ならリンクを出さない

        template <class Archive>
        void save(Archive& archive) const {
            archive(cereal::make_nvp("id", id),
                    cereal::make_nvp("title", title),
                    cereal::make_nvp("year", year),
                    cereal::make_nvp("yearLabel", yearLabel),
                    cereal::make_nvp("region", region),
                    cereal::make_nvp("category", category),
                    cereal::make_nvp("art", art),
                    cereal::make_nvp("difficulty", difficulty),
                    cereal::make_nvp("caption", caption),
                    cereal::make_nvp("wiki", wiki));
        }

        //! 読み込みは、難易度や説明の無いデッキも読めるようにする（難易度が無ければむずかしい扱い）。
        template <class Archive>
        void load(Archive& archive) {
            archive(cereal::make_nvp("id", id),
                    cereal::make_nvp("title", title),
                    cereal::make_nvp("year", year),
                    cereal::make_nvp("yearLabel", yearLabel),
                    cereal::make_nvp("region", region),
                    cereal::make_nvp("category", category),
                    cereal::make_nvp("art", art));
            Tsukino::IO::LoadOptional(archive, "difficulty", difficulty);
            Tsukino::IO::LoadOptional(archive, "caption", caption);
            Tsukino::IO::LoadOptional(archive, "wiki", wiki);
        }
    };

}    // namespace ChronoSort::Rules
