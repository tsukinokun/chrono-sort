//----------------------------------------------------------------------------
//! @file   PaperInsetTuning.hpp
//! @brief  紙片の画像ごとの、縁の欠け（ChronoSort/Assets/Data/Tuning/PaperInsets.json）
//! @detail 画像の四角のうち、縁が破れて透けている幅を、左・上・右・下の割合（x・y・z・w）で書きます。
//!         画像を描き直したら測り直します（縁まで不透明な、いちばん大きい内側の四角）。
//!         書いていない画像は欠け 0（四角いっぱいに書いてよい）として扱います。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/IO/CerealOptional.hpp>
#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>

#include <hlsl++.h>

#include <map>
#include <string>

namespace ChronoSort::UI {

    //! 紙片の画像ごとの、縁の欠けです。
    struct PaperInsetTuning {
        float                                 padding = 6.0f;    // 内側の四角から、さらに空ける余白（UI の座標）
        std::map<std::string, hlslpp::float4> insets  = {        // 画像 → 左・上・右・下の欠けの割合
            {"portrait_frame.png", hlslpp::float4(0.138f, 0.116f, 0.133f, 0.112f)},
            {"scrap_note.png", hlslpp::float4(0.078f, 0.196f, 0.076f, 0.190f)},
            {"scrap_result.png", hlslpp::float4(0.040f, 0.051f, 0.040f, 0.049f)},
            {"scrap_square.png", hlslpp::float4(0.092f, 0.107f, 0.092f, 0.104f)},
            {"scrap_tall.png", hlslpp::float4(0.107f, 0.081f, 0.107f, 0.078f)},
            {"scrap_wide.png", hlslpp::float4(0.058f, 0.167f, 0.056f, 0.162f)},
            {"title_logo.png", hlslpp::float4(0.050f, 0.145f, 0.048f, 0.142f)},
        };
    };

    //! PaperInsetTuning を読み込みます。
    template <class Archive>
    void load(Archive& archive, PaperInsetTuning& t) {
        Tsukino::IO::LoadOptional(archive, "padding", t.padding);
        Tsukino::IO::LoadOptional(archive, "insets", t.insets);
    }

}    // namespace ChronoSort::UI
