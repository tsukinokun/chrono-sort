//----------------------------------------------------------------------------
//! @file   BoardState.hpp
//! @brief  卓全体の並べ方の状態（レジストリのコンテキストに1つ置く）
//! @detail カード1枚ずつの置き場所は CardPlacementComponent に、卓全体で1つの状態
//!         （場の列の注目点・隙間・流す量、各プレイヤーの手札の見せ方、目印など）はここに持ちます。
//!         対局のシーンだけが置き、BoardLayoutSystem はこれが無いシーン（タイトルなど）では何もしません。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <hlsl++.h>

#include <optional>
#include <vector>

namespace ChronoSort::Board {

    //! 手札の見せ方です。
    enum class HandStyle {
        Fan,       // 手前の扇（人間）
        Seat,      // 卓の奥の席に並べる（CPU）
    };

    //! 1人分の手札の見せ方と、そのうち特別扱いしているカードです（カードそのものは CardPlacementComponent が持つ）。
    struct HandState {
        HandStyle      style     = HandStyle::Fan;                       // 並べ方
        hlslpp::float3 seat      = hlslpp::float3(0.0f, 0.0f, 0.0f);     // 席の中心（Seat のとき）
        int            hovered   = -1;                                   // カーソルが重なっている手札
        int            lifted    = -1;                                   // 持ち上げている手札（選択中・CPU の思考中）
        hlslpp::float2 hoverTilt = hlslpp::float2(0.0f, 0.0f);           // カーソルの位置へ傾ける量
    };

    //! 場の列の並び（流す前の座標）です。注目点の周りは広く、遠いほど狭く詰めます。
    struct RowLayout {
        std::vector<float> width;      // 各カードが占める幅
        std::vector<float> weight;     // 注目点への近さ（0〜1。拡大の強さを含む）
        float              focus    = 0.0f;    // 注目点（スロット k の境目が k）
        float              strength = 0.0f;    // 拡大の強さ（0〜1）
        float              farScale = 1.0f;    // 注目点から遠いカードの拡大率
        float              total    = 0.0f;    // 隙間を含む列の全体の幅
    };

    //! 卓全体の並べ方の状態です。
    struct BoardState {
        std::vector<HandState>            hands;                       // 各プレイヤーの手札の見せ方
        std::vector<Tsukino::ECS::Entity> deckCovers;                  // 山札の厚みを見せる裏向きのカード
        Tsukino::ECS::Entity              marker = entt::null;         // 挿入位置の目印
        Tsukino::ECS::Entity              held   = entt::null;         // 並べ直しから外すカード（ドラッグ中・公開中）
        std::optional<size_t>             gap;                         // 隙間を空けているスロット
        float                             gapWidth = 0.0f;             // 隙間の今の広さ（なめらかに開閉する）
        std::optional<float>              focusX;                      // 外から渡された注目点（卓の上の X 座標）
        float                             focus         = 0.0f;        // 今の注目点（なめらかに追う）
        float                             focusStrength = 0.0f;        // 今の拡大の強さ（なめらかに追う）
        RowLayout                         layout;                      // 今の並び（BoardLayoutSystem が毎フレーム求め直す）
        float                             scroll       = 0.0f;         // 場の列を流している今の量
        float                             scrollTarget = 0.0f;         // 場の列を流す量の目標
        float                             time       = 0.0f;           // 経過時間（揺れの演出用）
        int                               rowHovered = -1;             // 場の列でカーソルが重なっているカード
    };

}    // namespace ChronoSort::Board
