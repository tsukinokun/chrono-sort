//----------------------------------------------------------------------------
//! @file   RevealComponent.hpp
//! @brief  場に出したカードの公開と判定の演出の進み具合
//! @detail 出したカードに付け、RevealSystem が経過時間に合わせて段階を進めます
//!         （運ぶ → 裏返す → 叩きつける → 着地 → 判定 → 正解なら列へ／不正解なら正しい位置を見せて捨て札へ）。
//!         終わると外れます。演出の途中の状態がここに見えるので、ほかのシステムからも分かります。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/Match.hpp>

namespace ChronoSort::ECS {

    //! 公開と判定の演出の段階です（この順に進む）。
    enum class RevealStage {
        Carry,          // 隙間の上まで運ぶ
        Flip,           // 持ち上げながら裏返す
        Slam,           // 卓へ叩きつける
        Land,           // 着地（砂埃・揺れ）
        Judge,          // 正否を出す
        ShowCorrect,    // 不正解：正しい位置の上に掲げて見せる
        Discard,        // 不正解：捨て札へ送る
        Finish,         // 終わり（手札の補充と手番の終わりへ）
        Done,           // 片付け待ち
    };

    //! 場に出したカードの公開と判定の演出の進み具合です。
    struct RevealComponent {
        size_t            player  = 0;                    // 出したプレイヤー
        Rules::PlayResult result;                         // 置いた結果（判定は置いた瞬間に決まっている）
        RevealStage       stage   = RevealStage::Carry;   // 次に行う段階
        float             time    = 0.0f;                 // 出してからの時間（秒）
    };

}    // namespace ChronoSort::ECS
