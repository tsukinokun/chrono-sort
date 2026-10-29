//----------------------------------------------------------------------------
//! @file   SceneRouter.cpp
//! @brief  画面の切り替え先をまとめた窓口の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Scene/SceneRouter.hpp>

#include <ChronoSort/Scene/CreditsScene.hpp>
#include <ChronoSort/Scene/GameScene.hpp>
#include <ChronoSort/Scene/TitleScene.hpp>

#include <Tsukino/EngineIntegration/EngineAPI.hpp>

#include <Windows.h>
#include <memory>

namespace ChronoSort::Scene::Router {

    namespace {
        constexpr const char* kTitle   = "Title";      // タイトル画面の名前
        constexpr const char* kCredits = "Credits";    // クレジットの画面の名前
    }    // namespace

    //----------------------------------------------------------------------------
    //! 名前で切り替えるシーンを登録します。
    //----------------------------------------------------------------------------
    void RegisterScenes(Tsukino::EngineIntegration::EngineAPI& api) {
        api.RegisterScene(kTitle, [] { return std::make_unique<TitleScene>(); });
        api.RegisterScene(kCredits, [] { return std::make_unique<CreditsScene>(); });
    }

    //----------------------------------------------------------------------------
    //! タイトル画面へ移ります。
    //----------------------------------------------------------------------------
    void ToTitle(Tsukino::EngineIntegration::EngineAPI& api) {
        api.ChangeScene(kTitle);
    }

    //----------------------------------------------------------------------------
    //! 出典・クレジットの画面へ移ります。
    //----------------------------------------------------------------------------
    void ToCredits(Tsukino::EngineIntegration::EngineAPI& api) {
        api.ChangeScene(kCredits);
    }

    //----------------------------------------------------------------------------
    //! 対局の画面へ移ります。
    //----------------------------------------------------------------------------
    void ToMatch(Tsukino::EngineIntegration::EngineAPI& api, const Rules::MatchConfig& config) {
        api.ChangeScene(std::make_unique<GameScene>(config));
    }

    //----------------------------------------------------------------------------
    //! ソロの対局の設定を作ります。
    //----------------------------------------------------------------------------
    Rules::MatchConfig Solo(Rules::CardRange range) {
        Rules::MatchConfig config;
        config.mode  = Rules::MatchMode::Solo;
        config.range = range;
        return config;
    }

    //----------------------------------------------------------------------------
    //! CPU 対戦の設定を作ります。
    //----------------------------------------------------------------------------
    Rules::MatchConfig Versus(Rules::CardRange range, const std::vector<Rules::CpuLevel>& cpus) {
        Rules::MatchConfig config;
        config.mode  = Rules::MatchMode::Versus;
        config.cpus  = cpus;
        config.range = range;
        return config;
    }

    //----------------------------------------------------------------------------
    //! ゲームを終えます。
    //----------------------------------------------------------------------------
    void Quit() {
        // メインループは WM_QUIT を受けると抜ける
        PostQuitMessage(0);
    }

}    // namespace ChronoSort::Scene::Router
