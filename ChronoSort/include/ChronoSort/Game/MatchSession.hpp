//----------------------------------------------------------------------------
//! @file   MatchSession.hpp
//! @brief  1回の対局の状態（対局のシーンのレジストリのコンテキストに置く）
//! @detail 対局の進行のシステム（MatchFlow・CpuTurn・HumanInput・Reveal など）は、シーンではなく
//!         ここを読み書きして連携します。規則そのものは Rules::Match が持ち、ここはその周りの
//!         「今どの段階か」「次の段取りはいつか」「入力の途中の状態」と、卓・画面 UI・粒子の窓口を束ねます。
//!         窓口（BoardView など）の持ち主は GameScene で、ここは借りているだけです。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Game/Sequencer.hpp>
#include <ChronoSort/Rules/CpuPlayer.hpp>
#include <ChronoSort/Rules/Match.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <hlsl++.h>

#include <optional>
#include <random>
#include <utility>
#include <vector>

namespace Tsukino::ECS {
    class EventBus;
}    // namespace Tsukino::ECS

namespace ChronoSort::Board {
    class BoardView;
}    // namespace ChronoSort::Board

namespace ChronoSort::Effects {
    class ParticleEmitter;
}    // namespace ChronoSort::Effects

namespace ChronoSort::UI {
    class GameHud;
}    // namespace ChronoSort::UI

namespace ChronoSort::Game {

    //! 対局の段階です。
    enum class MatchPhase {
        Dealing,      // 配っている
        HumanTurn,    // 人間の手番（入力待ち）
        CpuTurn,      // CPU が考えている
        Resolving,    // 置いたカードの公開と判定の演出中
        Finished,     // 対局が終わった
    };

    //! 時間を置いてから行う、対局の進行の段取りです（同時に1つだけ）。
    enum class MatchStep {
        None,           // 待っている段取りは無い
        StartTurn,      // 手番を始める
        AdvanceTurn,    // 手番を次の人へ回す（終わっていれば結果へ）
        ShowResult,     // 結果の羊皮紙を出す
    };

    //! CPU が考えている間の段取りです。
    struct CpuThinking {
        bool                                 active = false;    // 考えている最中か
        size_t                               player = 0;        // 考えている CPU
        Rules::CpuMove                       move;              // 最後に置く手
        float                                time  = 0.0f;      // 考え始めてからの時間
        float                                think = 0.0f;      // 考える時間（自信が無いほど長い）
        std::vector<std::pair<float, size_t>> hops;             // 迷って指す置き先（時刻・スロット）
        size_t                               nextHop = 0;       // 次に指す迷い
    };

    //! 人間の手番の入力の途中の状態です。
    struct HumanInput {
        int                   grabbed      = -1;       // 押している手札（ドラッグ前を含む）
        int                   selected     = -1;       // クリックで選んだ手札
        bool                  dragging     = false;    // ドラッグ中か
        hlslpp::float2        pressPoint   = hlslpp::float2(0.0f, 0.0f);          // 押した画面位置
        hlslpp::float3        dragPoint    = hlslpp::float3(0.0f, 0.0f, 0.0f);    // ドラッグ中のカードの位置
        hlslpp::float3        dragVelocity = hlslpp::float3(0.0f, 0.0f, 0.0f);    // ドラッグの速度（傾きに使う）
        std::optional<size_t> preview;                 // 置こうとしているスロット
        float                 idleTime  = 0.0f;        // 人間の手番で操作していない時間（秒）
        hlslpp::float2        lastMouse = hlslpp::float2(0.0f, 0.0f);    // 前フレームのマウス位置
    };

    //! 1回の対局の状態です。
    struct MatchSession {
        Rules::MatchConfig config;              // 対局の設定
        Rules::Match       match;               // 規則の進行
        std::mt19937       rng;                 // 乱数（CPU の手・迷い方）
        MatchPhase         phase = MatchPhase::Dealing;    // 今の段階
        bool               paused      = false;           // 一時停止中か（段取りと入力を止める）
        bool               wasTiebreak = false;           // 直前まで延長戦だったか

        MatchStep step      = MatchStep::None;    // 時間を置いて行う段取り
        float     stepTimer = 0.0f;               // 段取りまでの残り時間

        CpuThinking cpu;      // CPU の思考
        HumanInput  input;    // 人間の入力

        Sequencer sequencer;    // 見た目だけの時間差（音の鳴らし分けなど）。進行の状態はここに積まない

        Tsukino::ECS::Entity       camera    = entt::null;    // 卓を見下ろすカメラ（マウスのレイに使う）
        Board::BoardView*          board     = nullptr;       // 卓の置き場所の窓口
        UI::GameHud*               hud       = nullptr;       // 画面 UI
        Effects::ParticleEmitter*  particles = nullptr;       // 粒子
        Tsukino::ECS::EventBus*    events    = nullptr;       // 対局の出来事を知らせる先

        //! 時間を置いて行う段取りを予約します（前の予約は上書きする）。
        //! @param  [in] next  段取り。
        //! @param  [in] delay 待つ時間（秒）。
        void Schedule(MatchStep next, float delay) {
            step      = next;
            stepTimer = delay;
        }
    };

}    // namespace ChronoSort::Game
