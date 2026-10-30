//----------------------------------------------------------------------------
//! @file   MatchTuning.hpp
//! @brief  対局の進め方と演出の調整値（ChronoSort/Assets/Data/Tuning/*.json）
//! @detail 公開の演出（Reveal.json）、人間の入力（Input.json）、CPU の迷い方（Cpu.json）、
//!         配り方と段取りの間（Deal.json）、出来事への返し方（Feedback.json）です。
//!         メンバの初期値が JSON を書く前の値で、JSON に書いていない項目はこの値のままです。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Game/TuningTypes.hpp>

#include <cereal/types/string.hpp>

#include <hlsl++.h>

#include <string>

namespace ChronoSort::Match {

    //! 出したカードの公開と判定の演出です（RevealSystem）。時刻は出してからの秒。
    struct RevealTuning {
        float flipAt           = 0.42f;    // 持ち上げながら裏返す
        float slamAt           = 0.98f;    // 卓へ叩きつける
        float landAt           = 1.10f;    // 着地
        float judgeAt          = 1.32f;    // 判定
        float correctDoneAfter = 0.8f;     // 正解：判定からこの秒数で次へ
        float showCorrectAfter = 0.75f;    // 不正解：判定からこの秒数で正しい位置を見せる
        float discardAfter     = 1.9f;     // 不正解：判定からこの秒数で捨て札へ
        float wrongDoneAfter   = 2.4f;     // 不正解：判定からこの秒数で次へ

        float          carryHeight          = 60.0f;                   // 隙間の上まで運ぶ高さ
        Game::Flight   carryFlight          = {0.38f, 35.0f, 0.0f};
        float          flipHeight           = 115.0f;                  // 裏返すときの高さ
        float          flipScale            = 1.1f;                    // 裏返すときの拡大率の倍率
        Game::Sharpness flipSharpness       = {9.0f, 7.0f};
        Game::Sharpness slamSharpness       = {34.0f, 16.0f};
        Game::Bump      landBump            = {0.3f, 0.07f};
        Game::Sharpness judgeSharpness      = {14.0f, 10.0f};
        float           showCorrectHeight   = 40.0f;                   // 正しい位置の上に浮かせる高さ
        Game::Sharpness showCorrectSharpness = {8.0f, 8.0f};
        Game::Sharpness discardSharpness    = {14.0f, 10.0f};
    };

    //! 人間の入力です（HumanInputSystem）。
    struct InputTuning {
        float clickThreshold     = 8.0f;      // これ以上動いたらクリックではなくドラッグ（ピクセル）
        float rowBandNear        = 130.0f;    // 場の列の手前、この距離までなら「列に置こうとしている」
        float rowBandFar         = 160.0f;    // 場の列の奥、この距離まで
        float edgePanMargin      = 0.08f;     // 画面端のこの割合の幅に入ると場の列が流れる
        float edgePanSpeed       = 420.0f;    // 画面端で列を流す速さ
        float wheelScroll        = 60.0f;     // ホイール1段で列を流す量
        float idleMouseThreshold = 2.0f;      // これより動いたら「操作している」とみなす（ピクセル）
        float idleNudgeSeconds   = 6.0f;      // 人間の手番でこの秒数だけ操作が無ければ、次の行動を促す
        int   tutorialMatches    = 3;         // カードを正しく置けた対局がこの数になるまで、遊び方の説明と手本を出す
        float demoMoveSeconds    = 1.1f;      // 手本：手札から隙間まで運ぶ時間
        float demoHoldSeconds    = 0.5f;      // 手本：隙間の上で止まる時間
        float demoRestSeconds    = 0.7f;      // 手本：次の手本までの間
        float demoArc            = 90.0f;     // 手本：運ぶときの弧の高さ（UI の座標）

        Game::Sharpness dragSharpness       = {28.0f, 16.0f};
        float           dragScale           = 1.05f;      // 持っているカードの拡大率
        float           dragLeanPerSpeed    = 0.0011f;    // 動かす速さに対する傾き
        float           dragLeanMax         = 0.4f;       // 傾きの上限（ラジアン）
        float           dragVelocitySmoothing = 12.0f;    // 傾きに使う速さのなまらせ方（大きいほど素早く追う）
        Game::Sharpness returnSharpness     = {14.0f, 10.0f};
        Game::Flight    returnFlight        = {0.35f, 30.0f, 0.0f};    // 置けない所で離したときに手札へ戻る
        Game::GlowSpec  selectGlow          = {hlslpp::float3(0.22f, 0.16f, 0.05f), 1.0f, 5.0f};    // クリックで選んだカードの脈打ち
    };

    //! CPU の迷い方の見せ方です（CpuTurnSystem）。置き先は Rules が決め、ここは考える時間と迷い方だけ。
    struct CpuTuning {
        float thinkBase     = 0.9f;     // 考える時間の基本（秒）
        float thinkPerDoubt = 1.6f;     // 自信の無さ（1 - 自信）に掛けて足す時間
        float thinkRandom   = 0.5f;     // 乱数で足す時間の上限
        int   hopsBase      = 1;        // 置き先の候補を行き来する回数の基本
        float hopsPerDoubt  = 3.0f;     // 自信の無さに掛けて足す回数
        float hopStart      = 0.35f;    // 考える時間のうち、行き来を始める割合
        float hopSpan       = 0.5f;     // 考える時間のうち、行き来に使う割合
        int   wander        = 1;        // 本命の置き先から何スロットまで迷うか
    };

    //! 配り方と、段取りの間です（MatchActions）。
    struct DealTuning {
        Game::Flight    openingFlight      = {0.7f, 120.0f, 0.2f};    // 場の1枚目
        float           openingHandsDelay  = 1.1f;                    // 場の1枚目のあと、手札を配り始めるまで
        Game::Flight    cardFlight         = {0.5f, 70.0f, 0.0f};     // 手札1枚（delay は使わない）
        float           cardStagger        = 0.09f;                   // 1枚ごとのずらし
        float           startTurnAfterDeal = 0.3f;                    // 配り終えてから最初の手番まで
        float           advanceAfterDeal   = 0.15f;                   // 補充を配り終えてから次の手番まで
        float           tiebreakDealDelay  = 0.8f;                    // 延長戦の手札を配り始めるまで
        float           tiebreakAfterDeal  = 0.2f;                    // 延長戦の手札を配り終えてから
        float           showResultDelay    = 1.4f;                    // 決着から結果の紙まで
        Game::Sharpness cancelSharpness    = {14.0f, 10.0f};          // 選択をやめたカードを戻す速さ
    };

    //! 対局の出来事への返し方です（MatchFeedbackSystem）。
    struct FeedbackTuning {
        float openingWhooshDelay = 0.2f;    // 場の1枚目：飛び立つ音まで
        float openingLandDelay   = 0.9f;    // 場の1枚目：降りて光るまで

        Game::GlowSpec openingGlow = {hlslpp::float3(0.55f, 0.38f, 0.1f), 1.6f, 1.4f};
        Game::GlowSpec correctGlow = {hlslpp::float3(0.55f, 0.38f, 0.1f), 1.4f, 1.1f};
        Game::GlowSpec wrongGlow   = {hlslpp::float3(0.6f, 0.05f, 0.03f), 1.0f, 0.6f};
        Game::GlowSpec winGlow     = {hlslpp::float3(0.55f, 0.38f, 0.1f), 1.5f, 1.2f};
        Game::Tremor   wrongTremor = {0.5f, 7.0f};
        Game::Bump     winBump     = {0.3f, 0.06f};
        float          winWaveStep = 0.05f;    // 勝ったとき、場の列を左から順に光らせる間隔

        float correctBurstHeight = 8.0f;    // 正解の火花を出す高さ
        float wrongBurstHeight   = 6.0f;    // 不正解の墨を出す高さ
        float landShake          = 4.0f;    // 着地のカメラの揺れ
        float wrongShake         = 7.0f;    // 不正解のカメラの揺れ

        // 帯の文言（UTF-8）
        std::string bannerYourTurn = "あなたの番";
        std::string bannerTiebreak = "延長戦！";
        std::string bannerWon      = "見事！";
        std::string bannerInkOut   = "インクが尽きた…";
        std::string bannerLost     = "惜しい…";
    };

#define CHRONOSORT_LOAD(name) Tsukino::IO::LoadOptional(archive, #name, t.name)

    //! RevealTuning を読み込みます。
    template <class Archive>
    void load(Archive& archive, RevealTuning& t) {
        CHRONOSORT_LOAD(flipAt);
        CHRONOSORT_LOAD(slamAt);
        CHRONOSORT_LOAD(landAt);
        CHRONOSORT_LOAD(judgeAt);
        CHRONOSORT_LOAD(correctDoneAfter);
        CHRONOSORT_LOAD(showCorrectAfter);
        CHRONOSORT_LOAD(discardAfter);
        CHRONOSORT_LOAD(wrongDoneAfter);
        CHRONOSORT_LOAD(carryHeight);
        CHRONOSORT_LOAD(carryFlight);
        CHRONOSORT_LOAD(flipHeight);
        CHRONOSORT_LOAD(flipScale);
        CHRONOSORT_LOAD(flipSharpness);
        CHRONOSORT_LOAD(slamSharpness);
        CHRONOSORT_LOAD(landBump);
        CHRONOSORT_LOAD(judgeSharpness);
        CHRONOSORT_LOAD(showCorrectHeight);
        CHRONOSORT_LOAD(showCorrectSharpness);
        CHRONOSORT_LOAD(discardSharpness);
    }

    //! InputTuning を読み込みます。
    template <class Archive>
    void load(Archive& archive, InputTuning& t) {
        CHRONOSORT_LOAD(clickThreshold);
        CHRONOSORT_LOAD(rowBandNear);
        CHRONOSORT_LOAD(rowBandFar);
        CHRONOSORT_LOAD(edgePanMargin);
        CHRONOSORT_LOAD(edgePanSpeed);
        CHRONOSORT_LOAD(wheelScroll);
        CHRONOSORT_LOAD(idleMouseThreshold);
        CHRONOSORT_LOAD(idleNudgeSeconds);
        CHRONOSORT_LOAD(tutorialMatches);
        CHRONOSORT_LOAD(demoMoveSeconds);
        CHRONOSORT_LOAD(demoHoldSeconds);
        CHRONOSORT_LOAD(demoRestSeconds);
        CHRONOSORT_LOAD(demoArc);
        CHRONOSORT_LOAD(dragSharpness);
        CHRONOSORT_LOAD(dragScale);
        CHRONOSORT_LOAD(dragLeanPerSpeed);
        CHRONOSORT_LOAD(dragLeanMax);
        CHRONOSORT_LOAD(dragVelocitySmoothing);
        CHRONOSORT_LOAD(returnSharpness);
        CHRONOSORT_LOAD(returnFlight);
        CHRONOSORT_LOAD(selectGlow);
    }

    //! CpuTuning を読み込みます。
    template <class Archive>
    void load(Archive& archive, CpuTuning& t) {
        CHRONOSORT_LOAD(thinkBase);
        CHRONOSORT_LOAD(thinkPerDoubt);
        CHRONOSORT_LOAD(thinkRandom);
        CHRONOSORT_LOAD(hopsBase);
        CHRONOSORT_LOAD(hopsPerDoubt);
        CHRONOSORT_LOAD(hopStart);
        CHRONOSORT_LOAD(hopSpan);
        CHRONOSORT_LOAD(wander);
    }

    //! DealTuning を読み込みます。
    template <class Archive>
    void load(Archive& archive, DealTuning& t) {
        CHRONOSORT_LOAD(openingFlight);
        CHRONOSORT_LOAD(openingHandsDelay);
        CHRONOSORT_LOAD(cardFlight);
        CHRONOSORT_LOAD(cardStagger);
        CHRONOSORT_LOAD(startTurnAfterDeal);
        CHRONOSORT_LOAD(advanceAfterDeal);
        CHRONOSORT_LOAD(tiebreakDealDelay);
        CHRONOSORT_LOAD(tiebreakAfterDeal);
        CHRONOSORT_LOAD(showResultDelay);
        CHRONOSORT_LOAD(cancelSharpness);
    }

    //! FeedbackTuning を読み込みます。
    template <class Archive>
    void load(Archive& archive, FeedbackTuning& t) {
        CHRONOSORT_LOAD(openingWhooshDelay);
        CHRONOSORT_LOAD(openingLandDelay);
        CHRONOSORT_LOAD(openingGlow);
        CHRONOSORT_LOAD(correctGlow);
        CHRONOSORT_LOAD(wrongGlow);
        CHRONOSORT_LOAD(winGlow);
        CHRONOSORT_LOAD(wrongTremor);
        CHRONOSORT_LOAD(winBump);
        CHRONOSORT_LOAD(winWaveStep);
        CHRONOSORT_LOAD(correctBurstHeight);
        CHRONOSORT_LOAD(wrongBurstHeight);
        CHRONOSORT_LOAD(landShake);
        CHRONOSORT_LOAD(wrongShake);
        CHRONOSORT_LOAD(bannerYourTurn);
        CHRONOSORT_LOAD(bannerTiebreak);
        CHRONOSORT_LOAD(bannerWon);
        CHRONOSORT_LOAD(bannerInkOut);
        CHRONOSORT_LOAD(bannerLost);
    }

#undef CHRONOSORT_LOAD

}    // namespace ChronoSort::Match
