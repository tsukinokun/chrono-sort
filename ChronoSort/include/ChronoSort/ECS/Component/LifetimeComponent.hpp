//----------------------------------------------------------------------------
//! @file   LifetimeComponent.hpp
//! @brief  時間が来たら子ごと片付ける印
//! @detail 通知の紙片・得点のポップ・滑り出した紙など、しばらく見せたら消す UI に付けます。
//!         LifetimeSystem が残り時間を減らし、0 になったらそのエンティティと Transform の子孫をまとめて破棄します。
//----------------------------------------------------------------------------
#pragma once

namespace ChronoSort::ECS {

    //! 時間が来たら子ごと片付ける印です。
    struct LifetimeComponent {
        float remaining = 0.0f;    // 残り時間（秒）
    };

}    // namespace ChronoSort::ECS
