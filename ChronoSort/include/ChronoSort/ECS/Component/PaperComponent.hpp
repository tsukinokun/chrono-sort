//----------------------------------------------------------------------------
//! @file   PaperComponent.hpp
//! @brief  紙片の、文字を書いてよい内側の四角
//! @detail 紙の画像は縁が破れて透けているので、目に見える紙は四角より小さくなります。
//!         PaperUI::Paper が、テクスチャごとの縁の欠け（Data/Tuning/PaperInsets.json）から内側の四角を求めて付けます。
//!         PaperUI::Text は、親がこれを持っていれば文字をこの四角の幅に収めます。
//----------------------------------------------------------------------------
#pragma once
#include <hlsl++.h>

#include <string>

namespace ChronoSort::ECS {

    //! 紙片の、文字を書いてよい内側の四角です（紙片のローカル座標）。
    struct PaperComponent {
        hlslpp::float2 safeMin = hlslpp::float2(0.0f, 0.0f);    // 左上
        hlslpp::float2 safeMax = hlslpp::float2(0.0f, 0.0f);    // 右下
        std::string    texture;                                  // 紙片の画像（はみ出しの警告に使う）
    };

}    // namespace ChronoSort::ECS
