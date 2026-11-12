//----------------------------------------------------------------------------
//! @file   ExternalLink.hpp
//! @brief  ゲームの外の記事を開く
//! @detail カードの出来事の Wikipedia の記事の URL を作り、既定のブラウザで開きます。
//----------------------------------------------------------------------------
#pragma once
#include <string>

namespace ChronoSort::Util {

    //! Wikipedia（日本語版）の記事の URL を作ります。
    //! @param  [in] title 記事名（UTF-8）。
    //! @return 記事の URL。空白は下線に、ASCII の英数字と - . _ ~ 以外はパーセント符号化する。
    std::string WikipediaUrl(const std::string& title);

    //! Wikipedia の検索の URL を作ります（記事名の分からない言語版で、出来事名から記事を探す）。
    //! @param  [in] words    検索語（UTF-8）。
    //! @param  [in] language 言語版のコード（"en" など）。
    //! @return 検索の URL。
    std::string WikipediaSearchUrl(const std::string& words, const std::string& language);

    //! URL を既定のブラウザで開きます。開けなければ警告のログを出します。
    //! @param  [in] url URL。
    void OpenInBrowser(const std::string& url);

}    // namespace ChronoSort::Util
