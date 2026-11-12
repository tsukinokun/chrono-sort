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

    //! ワイド文字列を UTF-8 文字列に変換します。
    //! @param  [in] wide ワイド文字列。
    //! @return 変換後の文字列。変換に失敗したら空文字列。
    inline std::string WideToUtf8(const std::wstring& wide) {
        if(wide.empty()) {
            return {};
        }
        const int length = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
        if(length <= 0) {
            return {};
        }
        std::string utf8(static_cast<size_t>(length), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), utf8.data(), length, nullptr, nullptr);
        return utf8;
    }

    //! 文を行に分けます。ラテン文字（アクセント付きを含む）などの半角の文字は全角の半分に数えます。
    //! 句読点や閉じ括弧は行頭に置かず（前の行にぶら下げる）、開き括弧は行末に置きません。
    //! 英単語や数字の並びは途中で切らず、空白の所で折り返します（行の頭と終わりの空白は捨てる）。
    //! 1行に入らないほど長い語だけは、語の途中で切ります。
    //! @param  [in] text         文。
    //! @param  [in] charsPerLine 1行の字数（全角）。
    //! @return 行の一覧。
    inline std::vector<std::wstring> WrapText(const std::wstring& text, size_t charsPerLine) {
        constexpr std::wstring_view kNoLineStart = L"、。，．）」』】〉》・ー？！";
        constexpr std::wstring_view kNoLineEnd   = L"（「『【〈《";
        const auto isHalfWidth = [](wchar_t c) { return c < 0x0250; };    // ASCII・Latin-1・ラテン文字拡張
        const auto isWordChar  = [&](wchar_t c) { return isHalfWidth(c) && c != L' '; };

        std::vector<std::wstring> lines;
        size_t                    start = 0;
        while(start < text.size()) {
            // 折り返した後の行は空白から始めない
            while(start < text.size() && text[start] == L' ') {
                ++start;
            }
            if(start >= text.size()) {
                break;
            }

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
                const size_t fitted = end;
                while(end > start + 1 && end < text.size() &&
                      (kNoLineEnd.find(text[end - 1]) != std::wstring_view::npos || (isWordChar(text[end - 1]) && isWordChar(text[end])))) {
                    --end;
                }
                // 行の頭から1語だけで入りきらないなら、語の途中で切るしかない
                if(end == start + 1 && isWordChar(text[start]) && isWordChar(text[end])) {
                    end = fitted;
                }
            }

            size_t last = end;
            while(last > start && text[last - 1] == L' ') {
                --last;
            }
            lines.push_back(text.substr(start, last - start));
            start = end;
        }
        return lines;
    }

}    // namespace ChronoSort::Util
