//----------------------------------------------------------------------------
//! @file   SceneRouter.hpp
//! @brief  画面の切り替え先をまとめた窓口
//! @detail どの画面からどの画面へ移るかは、ここの関数で行います。各シーンはほかのシーンのヘッダを読まずに済みます。
//!         タイトルとクレジットはエンジンの名前付きシーン（EngineAPI::RegisterScene）として登録し、名前で切り替えます。
//!         対局は設定（MatchConfig）を渡すので、ここで GameScene を作って切り替えます。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/Match.hpp>

#include <vector>

namespace Tsukino::EngineIntegration {
    class EngineAPI;
}    // namespace Tsukino::EngineIntegration

namespace ChronoSort::Scene::Router {

    //! 名前で切り替えるシーン（タイトル・クレジット）を登録します。起動時に1回呼びます。
    //! @param  [in,out] api エンジン API。
    void RegisterScenes(Tsukino::EngineIntegration::EngineAPI& api);

    //! タイトル画面へ移ります。
    //! @param  [in,out] api エンジン API。
    void ToTitle(Tsukino::EngineIntegration::EngineAPI& api);

    //! 出典・クレジットの画面へ移ります。
    //! @param  [in,out] api エンジン API。
    void ToCredits(Tsukino::EngineIntegration::EngineAPI& api);

    //! 対局の画面へ移ります。
    //! @param  [in,out] api    エンジン API。
    //! @param  [in]     config 対局の設定（Solo・Versus で作る）。
    void ToMatch(Tsukino::EngineIntegration::EngineAPI& api, const Rules::MatchConfig& config);

    //! ソロの対局の設定を作ります。
    //! @param  [in] range 出題の範囲。
    //! @return 設定。
    Rules::MatchConfig Solo(Rules::CardRange range);

    //! CPU 対戦の設定を作ります。
    //! @param  [in] range 出題の範囲。
    //! @param  [in] cpus  相手の CPU の強さ（席の順。人数はこの数）。
    //! @return 設定。
    Rules::MatchConfig Versus(Rules::CardRange range, const std::vector<Rules::CpuLevel>& cpus);

    //! ゲームを終えます（メインループが抜ける）。
    void Quit();

}    // namespace ChronoSort::Scene::Router
