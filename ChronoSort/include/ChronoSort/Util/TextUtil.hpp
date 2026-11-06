//----------------------------------------------------------------------------
//! @file   TextUtil.hpp
//! @brief  文字列変換の小道具
//! @detail デッキ JSON は UTF-8、FontComponent は std::wstring を受け取るため、
//!         その間の変換を受け持ちます。FontComponent は1行しか描かないので、長い文の行分けもここで行います。
//----------------------------------------------------------------------------
#pragma once
#include <string>
#include <string_view>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace ChronoSort::Util {

    //! UTF-8 文字列をワイド文字列に変換します。
    //! @param  [in] utf8 UTF-8 文字列。
    //! @return 変換後の文字列。変換に失敗したら空文字列。
    inline std::wstring Utf8ToWide(const std::string& utf8) {
        if(utf8.empty()) {
            return {};
        }
        const int length = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
        if(length <= 0) {
            return {};
        }
        std::wstring wide(static_cast<size_t>(length), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), length);
        return wide;
    }

    //! 文を行に分けます。半角の文字は全角の半分に数えます。
    //! 句読点や閉じ括弧は行頭に置かず（前の行にぶら下げる）、開き括弧は行末に置かず、英数字の並びは切りません。
    //! @param  [in] text         文。
    //! @param  [in] charsPerLine 1行の字数（全角）。
    //! @return 行の一覧。
    inline std::vector<std::wstring> WrapText(const std::wstring& text, size_t charsPerLine) {
        constexpr std::wstring_view kNoLineStart = L"、。，．）」』】〉》・ー？！";
        constexpr std::wstring_view kNoLineEnd   = L"（「『【〈《";
        const auto isHalfWidth = [](wchar_t c) { return c < 0x80; };

        std::vector<std::wstring> lines;
        size_t                    start = 0;
        while(start < text.size()) {
            // 1行に入るところまで進める
            size_t end   = start;
            size_t width = 0;    // 半角で数えた幅
            while(end < text.size() && width + (isHalfWidth(text[end]) ? 1 : 2) <= charsPerLine * 2) {
                width += isHalfWidth(text[end]) ? 1 : 2;
                ++end;
            }
            if(end == start) {
                ++end;
            }
            if(end < text.size()) {
                while(end < text.size() && kNoLineStart.find(text[end]) != std::wstring_view::npos) {
                    ++end;
                }
                while(end > start + 1 && end < text.size() &&
                      (kNoLineEnd.find(text[end - 1]) != std::wstring_view::npos || (isHalfWidth(text[end - 1]) && isHalfWidth(text[end])))) {
                    --end;
                }
            }
            lines.push_back(text.substr(start, end - start));
            start = end;
        }
        return lines;
    }

}    // namespace ChronoSort::Util
