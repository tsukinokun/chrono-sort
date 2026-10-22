//----------------------------------------------------------------------------
//! @file   CardData.hpp
//! @brief  歴史カード1枚分のデータ
//! @detail デッキ JSON から読み込む値そのものです。表示や ECS には依存しません。
//----------------------------------------------------------------------------
#pragma once
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>

#include <cstdint>
#include <string>

namespace ChronoSort::Rules {

    //! 歴史カード1枚分のデータです。
    struct CardData {
        std::string  id;           // 一意な識別子（画像ファイル名にも使う）
        std::string  title;        // 出来事の名前（UTF-8）
        std::int32_t year = 0;     // 西暦年。紀元前は負の数。並び順の判定はこの値だけで行う
        std::string  yearLabel;    // 画面に出す年号表記（例：「紀元前221年」）
        std::string  region;       // 地域（"JP" / "WORLD"）
        std::string  category;     // 分類（政治・文化など）
        std::string  image;        // カード絵のパス（ワークスペースルートからの相対）

        template <class Archive>
        void serialize(Archive& archive) {
            archive(cereal::make_nvp("id", id),
                    cereal::make_nvp("title", title),
                    cereal::make_nvp("year", year),
                    cereal::make_nvp("yearLabel", yearLabel),
                    cereal::make_nvp("region", region),
                    cereal::make_nvp("category", category),
                    cereal::make_nvp("image", image));
        }
    };

}    // namespace ChronoSort::Rules
