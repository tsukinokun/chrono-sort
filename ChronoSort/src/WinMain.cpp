//----------------------------------------------------------------------------
//! @file   WinMain.cpp
//! @brief  ChronoSort のエントリポイント
//----------------------------------------------------------------------------
#include <ChronoSort/Game/FrameMonitor.hpp>
#include <ChronoSort/Game/GameServices.hpp>
#include <ChronoSort/Game/Prefabs.hpp>
#include <ChronoSort/Rules/RulesSelfTest.hpp>
#include <ChronoSort/Scene/GameScene.hpp>
#include <ChronoSort/Scene/LoadingScene.hpp>
#include <ChronoSort/Scene/SceneRouter.hpp>
#include <ChronoSort/Scene/TitleScene.hpp>

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineIntegration.hpp>

#include <Windows.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>

//----------------------------------------------------------------------------
//! アプリケーションのエントリポイントです。
//! @return 終了コード。
//----------------------------------------------------------------------------
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR lpCmdLine, _In_ int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // デバッガの外で起動したときも警告が見えるよう、ログをファイルにも出す
    Tsukino::Core::Log::SetLogFile("Logs/ChronoSort.log");

#ifdef _DEBUG
    // ルール判定の境界ケースを起動のたびに確かめる（結果はログへ）
    ChronoSort::Rules::RunSelfTest();
#endif

    Tsukino::EngineIntegration::EngineIntegration engineIntegration;
    if(!engineIntegration.Initialize(1600, 900, "ChronoSort")) {
        Tsukino::Core::Log::Error("Failed to initialize EngineIntegration.");
        return EXIT_FAILURE;
    }

    // ゲームのコンポーネントを Prefab から作れるようにする（最初のシーンより前に）
    ChronoSort::Game::Prefabs::RegisterComponents(*engineIntegration.GetContext().prefabFactory);

    // シーンをまたいで共有する音と設定。前回の設定（音量など）は、最初の音が鳴る前にここで反映される。
    // エンジンより後に作るので、エンジンより先に壊れる（音の後片付けがエンジンの片付けより前に済む）
    ChronoSort::Game::GameServices services(engineIntegration.GetContext());

    Tsukino::EngineIntegration::EngineAPI engineAPI(engineIntegration.GetContext());
    ChronoSort::Scene::Router::RegisterScenes(engineAPI);
    // 最初に読込画面でアセットをまとめて読み、その後の画面切替や操作で止まらないようにする。
    // 「ChronoSort.exe solo」「ChronoSort.exe versus」なら、読み終えたあとタイトルを飛ばして対局画面へ（確認用）
    // 続けて japan / world を書くと、出題をその範囲に絞る（例：「ChronoSort.exe solo japan」）
    const std::string argument = (lpCmdLine != nullptr) ? lpCmdLine : "";
    auto next = [argument]() -> std::unique_ptr<Tsukino::EngineIntegration::GameSceneBase> {
        namespace Router = ChronoSort::Scene::Router;
        using ChronoSort::Rules::CardRange;
        using ChronoSort::Rules::CpuLevel;
        CardRange range = CardRange::All;
        if(argument.find("japan") != std::string::npos) {
            range = CardRange::Japan;
        } else if(argument.find("world") != std::string::npos) {
            range = CardRange::World;
        }
        if(argument.find("solo") != std::string::npos) {
            return std::make_unique<ChronoSort::Scene::GameScene>(Router::Solo(range));
        }
        if(argument.find("versus") != std::string::npos) {
            return std::make_unique<ChronoSort::Scene::GameScene>(Router::Versus(range, {CpuLevel::Easy, CpuLevel::Normal}));
        }
        return std::make_unique<ChronoSort::Scene::TitleScene>();
    };
    engineAPI.ChangeScene(std::make_unique<ChronoSort::Scene::LoadingScene>(next));

    // deltaTime は実測値を使う（高リフレッシュレート環境で演出が速くならないように）
    auto lastTime = std::chrono::steady_clock::now();
    while(engineAPI.ProcessMessages()) {
        auto  currentTime = std::chrono::steady_clock::now();
        float deltaTime   = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime          = currentTime;
        // 上限で丸める前の実測で、引っかかりを記録する
        ChronoSort::Game::FrameMonitor::Tick(deltaTime);
        // ウィンドウドラッグ等で1フレームが極端に長くなったときの暴走防止
        deltaTime = std::min(deltaTime, 1.0f / 15.0f);

        engineAPI.Update(deltaTime);
        engineAPI.Render();
    }

    ChronoSort::Game::FrameMonitor::Report();
    return 0;
}
