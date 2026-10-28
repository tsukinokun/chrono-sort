//----------------------------------------------------------------------------
//! @file   CardPlacementComponent.hpp
//! @brief  卓の上でカードが属している場所
//! @detail 「場の列の何枚目」「誰の手札の何枚目」「捨て札の何枚目」をカードごとに持ちます。
//!         BoardLayoutSystem がこれを読んで、毎フレーム目標の姿勢を CardMotion に書きます。
//!         並べ替え（列への挿入・手札から出す）は BoardView を通して行い、番号の詰め直しもそこで行います。
//----------------------------------------------------------------------------
#pragma once
#include <hlsl++.h>

#include <cstddef>

namespace ChronoSort::ECS {

    //! カードの置き場所の種類です。
    enum class CardZone {
        None,       // どこにも並べない（公開の演出中・山札から飛んでいる途中など）
        Row,        // 場の列（年代順）
        Hand,       // 手札
        Discard,    // 捨て札の山
    };

    //! 卓の上でカードが属している場所です。
    struct CardPlacementComponent {
        CardZone       zone   = CardZone::None;    // 置き場所
        size_t         owner  = 0;                 // 手札の持ち主（zone が Hand のとき）
        size_t         index  = 0;                 // 置き場所の中での順番
        hlslpp::float4 jitter = hlslpp::float4(0.0f, 0.0f, 0.0f, 0.0f);    // 捨て札のずれ（x, z, 回転, 未使用）
    };

}    // namespace ChronoSort::ECS
