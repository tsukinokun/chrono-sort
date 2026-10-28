//----------------------------------------------------------------------------
//! @file   ChoiceTagComponent.hpp
//! @brief  組の中から1つを選ぶ札（選択肢）の見た目
//! @detail エンジンの UIToggleComponent（組で1つだけ入になる）と一緒に、札の紙片に付けます。
//!         ChoiceTagSystem が、入になった札の文字を chosenColor にして弾ませ、ほかの札の文字を otherColor にします。
//!         どの札が選ばれているかは、UI::SelectedChoice で value として受け取ります。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <hlsl++.h>

namespace ChronoSort::ECS {

    //! 組の中から1つを選ぶ札の見た目です。
    struct ChoiceTagComponent {
        Tsukino::ECS::Entity text        = entt::null;    // 札の文字
        int                  value       = 0;             // この札を選んだときの値
        hlslpp::float4       chosenColor = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);    // 選ばれているときの文字の色
        hlslpp::float4       otherColor  = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);    // 選ばれていないときの文字の色
        float                punch       = 0.2f;          // 選ばれたときに弾む強さ（0 で弾まない）
        bool                 punchOnCreate = false;       // 作った直後も、選ばれている札を弾ませるか（開いた紙の今の選択を見せる）
        bool                 painted     = false;         // 一度でも色を塗ったか（作った直後に塗るため）
    };

    //! 選ばれた札ほど持ち上がる入れ物（しおりなど）の印です。
    //! @note   札の紙片の親に付ける（揺れ・ボタンの反応は札の側に残し、持ち上げは入れ物を動かして行う）。
    //!         ChoiceTagSystem が、tag の UIToggleComponent が入なら lift を 1 へ、切なら 0 へなめらかに寄せる。
    struct ChoiceLiftComponent {
        Tsukino::ECS::Entity tag          = entt::null;                      // 選ばれたかを見る札
        hlslpp::float2       base         = hlslpp::float2(0.0f, 0.0f);      // 置いた位置
        float                lift         = 0.0f;                            // 持ち上がり（0〜1）
        float                rate         = 12.0f;                           // 寄せる速さ
        float                restOffset   = 5.0f;                            // 選ばれていないときに沈める量
        float                liftOffset   = 21.0f;                           // 選ばれたときに持ち上げる量（restOffset からの差）
        float                restScale    = 0.93f;                           // 選ばれていないときの拡大率
        float                liftScale    = 0.14f;                           // 選ばれたときに足す拡大率
    };

}    // namespace ChronoSort::ECS
