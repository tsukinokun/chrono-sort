//----------------------------------------------------------------------------
//! @file   MatchEvents.hpp
//! @brief  対局の出来事（シーンの EventBus で知らせる）
//! @detail 進行のシステム（MatchFlow・Reveal など）は出来事を知らせるだけにし、
//!         音・粒子・画面 UI・カメラの揺れは MatchFeedbackSystem がそれぞれ受け取って鳴らします。
//!         どれも同期で即座に届きます。受け取った側で同じ型の出来事を出し直してはいけません（EventBus が止める）。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/Match.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <hlsl++.h>

namespace ChronoSort::Match {

    //! 対局の始まりに、場の1枚目を山札から飛ばしました（空中で裏返って列に降りる）。
    struct OpeningCardDealtEvent {
        Tsukino::ECS::Entity card = entt::null;
    };

    //! 手番が始まりました。
    struct TurnStartedEvent {
        size_t player = 0;        // 手番のプレイヤー
        bool   human  = false;    // 人間の手番か
    };

    //! 延長戦が始まりました。
    struct TiebreakStartedEvent {};

    //! CPU が手に取って考え始めました。
    struct CpuThinkingStartedEvent {
        size_t player = 0;    // 考えている CPU
    };

    //! CPU が迷って、置き先の候補を指しました。
    struct CpuHoveredSlotEvent {
        size_t slot = 0;    // 指した候補
    };

    //! 手札のカードが場へ出されました（ここから公開の演出が始まる）。
    struct CardCommittedEvent {
        size_t               player = 0;             // 出したプレイヤー
        Tsukino::ECS::Entity card   = entt::null;    // 出したカード
    };

    //! 出したカードが持ち上がって裏返りました。
    struct CardFlippedEvent {
        Tsukino::ECS::Entity card = entt::null;
    };

    //! 出したカードが卓に叩きつけられました。
    struct CardLandedEvent {
        Tsukino::ECS::Entity card = entt::null;
        hlslpp::float3       at   = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 着地した場所（卓の上）
    };

    //! 出したカードの正否が出ました。
    struct PlayJudgedEvent {
        size_t               player = 0;             // 出したプレイヤー
        Tsukino::ECS::Entity card   = entt::null;    // 出したカード
        Rules::PlayResult    result;                 // 結果
        hlslpp::float3       at = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 置いた隙間の場所（卓の上）
    };

    //! 間違えたカードを、本来の位置の上に掲げて見せました。
    struct CorrectSlotShownEvent {
        Tsukino::ECS::Entity card = entt::null;
    };

    //! 間違えたカードを捨て札の山へ送りました。
    struct CardDiscardedEvent {
        Tsukino::ECS::Entity card = entt::null;
    };

    //! 手札を山札から配りました（1枚ごと）。
    struct CardDealtEvent {
        float delay = 0.0f;    // そのカードが山札を離れるまでの待ち（秒）
    };

    //! 対局が終わりました。
    struct MatchEndedEvent {
        Rules::MatchOutcome outcome   = Rules::MatchOutcome::InProgress;    // 終わり方
        bool                won       = false;    // 人間が勝った（ソロならクリアした）か
        bool                newRecord = false;    // ソロの新記録か
    };

    //! 結果の羊皮紙を出す時が来ました。
    struct ResultRequestedEvent {};

}    // namespace ChronoSort::Match
