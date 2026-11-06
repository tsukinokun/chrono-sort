//----------------------------------------------------------------------------
//! @file   ExternalLink.cpp
//! @brief  ゲームの外の記事を開く処理の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Util/ExternalLink.hpp>

#include <ChronoSort/Util/TextUtil.hpp>

#include <Tsukino/Core/Log.hpp>

#include <shellapi.h>

namespace ChronoSort::Util {

    //----------------------------------------------------------------------------
    //! Wikipedia（日本語版）の記事の URL を作ります。
    //----------------------------------------------------------------------------
    std::string WikipediaUrl(const std::string& title) {
        constexpr char kHex[] = "0123456789ABCDEF";
        std::string    url    = "https://ja.wikipedia.org/wiki/";
        for(const char c : title) {
            const auto byte = static_cast<unsigned char>(c);
            if(c == ' ') {
                url += '_';
            } else if((byte >= '0' && byte <= '9') || (byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z') || c == '-' || c == '.' || c == '_' || c == '~') {
                url += c;
            } else {
                url += '%';
                url += kHex[byte >> 4];
                url += kHex[byte & 0x0F];
            }
        }
        return url;
    }

    //----------------------------------------------------------------------------
    //! URL を既定のブラウザで開きます。
    //----------------------------------------------------------------------------
    void OpenInBrowser(const std::string& url) {
        const std::wstring wide   = Utf8ToWide(url);
        const auto         result = reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", wide.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
        // 32 以下は失敗（ShellExecute の決まり）
        if(result <= 32) {
            Tsukino::Core::Log::Warn("Failed to open " + url + " (" + std::to_string(result) + ")");
        }
    }

}    // namespace ChronoSort::Util
