//----------------------------------------------------------------------------
//! @file   Credits.hpp
//! @brief  出典の一覧（ChronoSort/Assets/Data/Credits.json）
//! @detail カードの絵と音楽の出典を読みます。クレジット画面と、対局の後に出たカードを見る紙が使います。
//----------------------------------------------------------------------------
#pragma once
#include <string>
#include <vector>

namespace ChronoSort::Util {

    //! 出典の1件分です。
    struct CreditEntry {
        std::string id;            // カードの id（音楽は空）
        std::string title;         // 題名
        std::string file;          // 元のファイル名
        std::string page;          // 元のページの URL
        std::string artist;        // 作者・演奏者
        std::string license;       // ライセンス
        std::string licenseUrl;    // ライセンスの URL
    };

    //! 出典の一覧です。
    struct Credits {
        std::vector<CreditEntry> images;    // カードの絵
        std::vector<CreditEntry> audio;     // 音楽

        //! カードの絵の出典を、カードの id で探します。
        //! @param  [in] id カードの id。
        //! @return 見つかった出典。無ければ nullptr。
        const CreditEntry* FindImage(const std::string& id) const;
    };

    //! Credits.json を読みます。読めなければ空の一覧を返し、警告を出します。
    //! @return 出典の一覧。
    Credits LoadCredits();

}    // namespace ChronoSort::Util
