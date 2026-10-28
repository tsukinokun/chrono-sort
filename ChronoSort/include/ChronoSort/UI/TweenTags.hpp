//----------------------------------------------------------------------------
//! @file   TweenTags.hpp
//! @brief  ゲームの画面 UI が使うトゥイーンの目印の一覧
//! @detail 同じ目印のトゥイーンを再生し直すと置き換わるので、用途ごとに別の値にします。
//!         滑り込み・消える・弾みはエンジンの定型（UITweenUtility::Tag。0xFFFFFF00 から上）を使うので、ここには無い。
//----------------------------------------------------------------------------
#pragma once
#include <cstdint>

namespace ChronoSort::UI {

    //! 画面 UI のトゥイーンの目印です。
    enum class TweenTag : std::uint32_t {
        Sway = 1,     // 紙片の常時の揺れ
        Bob,          // 紙片の常時の上下の漂い
        InkTip = 20,  // インク壺が倒れる
        InkSlide,     // 倒れたインク壺が下へずれる
        StampPress = 30,    // 朱印を押し付ける
        StampInk,           // 朱印のインクが乗る
        StampFade,          // 朱印が消える
        StampGrow,          // 朱印が広がる
        StampRecoil,        // 朱印の揺れ戻り
        StampBleed,         // 朱印の滲み
        PopupRise = 40,     // 得点が浮き上がる
        PopupPop,           // 得点が現れる
        PopupFade,          // 得点が消える
        ResultSealPress = 50,    // 結果のランクの印を押す
        ResultSealAppear,        // 結果のランクの印が現れる
        NewRecordBlink,          // 新記録の点滅
        LifeWarning = 60,        // 最後のインク壺の震え
        DeckWarning,             // 山札の札の弾み
        TurnBreathe,             // 手番の名札の呼吸
        IdleNudge,               // 放置の催促
    };

    //! 目印をエンジンのトゥイーンの tag にします。
    //! @param  [in] tag 目印。
    //! @return tag の値。
    constexpr std::uint32_t Tag(TweenTag tag) {
        return static_cast<std::uint32_t>(tag);
    }

}    // namespace ChronoSort::UI
