//----------------------------------------------------------------------------
//! @file   MatchActions.hpp
//! @brief  対局を進める操作（配る・手番を始める・カードを出す・終える）
//! @detail 進行のシステム（MatchFlow・CpuTurn・HumanInput・Reveal）が共通に使う操作です。
//!         どれもレジストリの MatchSession を読み書きし、起きたことは MatchEvents で知らせます。
//!         音や粒子は直接鳴らしません（MatchFeedbackSystem が出来事を受けて鳴らす）。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Game/MatchSession.hpp>

#include <Tsukino/Core/ECS/Registry/Registry.hpp>

namespace ChronoSort::Game {

    //! レジストリに置いた対局の状態を返します。
    //! @param  [in] registry 対局のシーンのレジストリ。
    //! @return 対局の状態。
    MatchSession& Session(Tsukino::ECS::Registry& registry);

    //! レジストリが対局の状態を持っているかを返します（対局のシーンだけが持つ）。
    //! @param  [in] registry レジストリ。
    //! @return 持っていれば true。
    bool HasSession(Tsukino::ECS::Registry& registry);

    //! 場の1枚目と手札を、山札から順に配ります（対局の始まり）。
    //! @param  [in,out] registry レジストリ。
    void DealOpening(Tsukino::ECS::Registry& registry);

    //! 手札の見た目を Rules::Match の手札に合わせます（足りない分を山札から1枚ずつ順に配る）。
    //! @param  [in,out] registry レジストリ。
    //! @param  [in]     delay    配り始めるまでの待ち（秒）。
    //! @return 最後のカードが届くまでの時間（秒）。
    float DealToHands(Tsukino::ECS::Registry& registry, float delay);

    //! 手番を始めます（終わっていれば結果へ）。
    //! @param  [in,out] registry レジストリ。
    void StartTurn(Tsukino::ECS::Registry& registry);

    //! 手札のカードを場に出し、公開と判定の演出を始めます。
    //! @param  [in,out] registry  レジストリ。
    //! @param  [in]     player    プレイヤー番号。
    //! @param  [in]     handIndex 手札の番号。
    //! @param  [in]     slot      置くスロット。
    void CommitPlay(Tsukino::ECS::Registry& registry, size_t player, size_t handIndex, size_t slot);

    //! 判定の演出のあと、手札を補充して手番を終えます。
    //! @param  [in,out] registry レジストリ。
    void FinishPlay(Tsukino::ECS::Registry& registry);

    //! 手番を次へ回します（終わっていれば結果へ。延長戦に入ったら知らせて追加の1枚を配る）。
    //! @param  [in,out] registry レジストリ。
    void AdvanceTurn(Tsukino::ECS::Registry& registry);

    //! 対局を終え、記録を保存して結果の演出を始めます。
    //! @param  [in,out] registry レジストリ。
    void EndMatch(Tsukino::ECS::Registry& registry);

    //! 人間の手番の選択とドラッグを解除し、カードを手札へ戻します。
    //! @param  [in,out] registry レジストリ。
    void CancelSelection(Tsukino::ECS::Registry& registry);

}    // namespace ChronoSort::Game
