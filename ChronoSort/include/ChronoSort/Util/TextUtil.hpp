//----------------------------------------------------------------------------
//! @file   TextUtil.hpp
//! @brief  文字列変換の小道具
//! @detail デッキ JSON は UTF-8、FontComponent は std::wstring を受け取るため、
//!         その間の変換を受け持ちます。
//----------------------------------------------------------------------------
#pragma once
#include <string>

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

}    // namespace ChronoSort::Util
