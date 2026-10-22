//----------------------------------------------------------------------------
//! @file   WinMain.cpp
//! @brief  ChronoSort のエントリポイント
//----------------------------------------------------------------------------
#include <ChronoSort/Rules/RulesSelfTest.hpp>
#include <ChronoSort/Scene/SoloGameScene.hpp>
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

    Tsukino::EngineIntegration::EngineAPI engineAPI(engineIntegration.GetContext());
    // 「ChronoSort.exe solo」でタイトルを飛ばして対局画面から始める（描画確認用）
    const std::string argument = (lpCmdLine != nullptr) ? lpCmdLine : "";
    if(argument.find("solo") != std::string::npos) {
        engineAPI.ChangeScene(std::make_unique<ChronoSort::Scene::SoloGameScene>());
    } else {
        engineAPI.ChangeScene(std::make_unique<ChronoSort::Scene::TitleScene>());
    }

    // deltaTime は実測値を使う（高リフレッシュレート環境で演出が速くならないように）
    auto lastTime = std::chrono::steady_clock::now();
    while(engineAPI.ProcessMessages()) {
        auto  currentTime = std::chrono::steady_clock::now();
        float deltaTime   = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime          = currentTime;
        // ウィンドウドラッグ等で1フレームが極端に長くなったときの暴走防止
        deltaTime = std::min(deltaTime, 1.0f / 15.0f);

        engineAPI.Update(deltaTime);
        engineAPI.Render();
    }

    return 0;
}
