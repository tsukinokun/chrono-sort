//----------------------------------------------------------------------------
//! @file   SystemSetup.hpp
//! @brief  シーンに並べるシステムの組み合わせと、更新の順番
//! @detail システムの登録を各シーンで手書きすると、順番の意味が散らばり、並べ忘れも起きます。
//!         ここに「3D の卓の画面」と「画面 UI だけの画面」の2通りをまとめ、順番には名前を付けます。
//!         小さい順に更新します。ゲームのシステム（対局の進行など）は kGameplay に足します。
//----------------------------------------------------------------------------
#pragma once

namespace Tsukino::ECS {
    class Scene;
}    // namespace Tsukino::ECS

namespace ChronoSort::ECS {

    //! システムの更新の順番です（小さい順に更新する）。
    namespace Order {
        constexpr int kInput       = -4;    // 画面 UI のマウス判定（その結果をこのフレームのうちに使う）
        constexpr int kUIAnimation = -3;    // 画面 UI のトゥイーン（Transform の計算より前に動きを確定させる）
        constexpr int kMatch       = -2;    // 対局の進行（段取り・CPU・人間の入力・公開の演出・カメラ）。並べ方が読む状態を先に決める
        constexpr int kGameplay    = -1;    // 卓の並べ方・カードの動き・粒子・発光（同じく Transform より前）
        constexpr int kTransform   = 0;     // 親子の Transform の計算
        constexpr int kCamera      = 5;     // カメラの行列
        constexpr int kAnchoredUI  = 6;     // 3D に貼り付く画面 UI（カメラの行列が決まってから）
        constexpr int kText        = 9;     // 文字の描画
        constexpr int kDraw        = 10;    // スプライト・モデルの描画
        constexpr int kLights      = 13;    // 光
        constexpr int kSky         = 14;    // 空（環境光の元）
        constexpr int kPostProcess = 15;    // 後処理（Bloom）
    }    // namespace Order

    //! 3D の卓の画面（対局・タイトル・クレジット）のシステムを並べます。
    //! カードの動き・粒子・発光も含みます。対局の進行のシステムは、これに加えて各シーンが足します。
    //! @param  [in,out] scene 並べる先のシーン。
    void AddTableSystems(Tsukino::ECS::Scene& scene);

    //! 対局を進めるシステムを並べます（AddTableSystems に加えて、対局のシーンだけが呼ぶ）。
    //! レジストリに MatchSession を置いてから呼びます。
    //! @param  [in,out] scene 並べる先のシーン。
    void AddMatchSystems(Tsukino::ECS::Scene& scene);

    //! 画面 UI だけの画面（読込画面）のシステムを並べます。マウスの判定は含みません。
    //! @param  [in,out] scene 並べる先のシーン。
    void AddUIOnlySystems(Tsukino::ECS::Scene& scene);

}    // namespace ChronoSort::ECS
