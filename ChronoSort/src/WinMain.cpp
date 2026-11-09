//----------------------------------------------------------------------------
//! @file   WinMain.cpp
//! @brief  ChronoSort のエントリポイント
//----------------------------------------------------------------------------
#include <ChronoSort/Game/FrameMonitor.hpp>
#include <ChronoSort/Game/GameServices.hpp>
#include <ChronoSort/Game/Prefabs.hpp>
#include <ChronoSort/Rules/CpuBalance.hpp>
#include <ChronoSort/Rules/Deck.hpp>
#include <ChronoSort/Rules/RulesSelfTest.hpp>
#include <ChronoSort/Scene/GameScene.hpp>
#include <ChronoSort/Scene/LoadingScene.hpp>
#include <ChronoSort/Scene/SceneRouter.hpp>
#include <ChronoSort/Scene/TitleScene.hpp>
#include <ChronoSort/Util/AssetPaths.hpp>

#include <Tsukino/Core/IO/JsonFile.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineIntegration.hpp>

#include <Windows.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <optional>
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

    // 「ChronoSort.exe cpubalance」なら、対戦 CPU の強さを測ってログへ出し、そのまま終わる（調整用）。
    // 時間がかかるので、窓を作る前に回す（窓が応答なしとして閉じられないように）
    if(lpCmdLine != nullptr && std::string(lpCmdLine).find("cpubalance") != std::string::npos) {
        ChronoSort::Rules::RulesTuning rules;
        Tsukino::IO::TryLoadJson(ChronoSort::Util::AssetPaths::Tuning("Rules"), "tuning", rules);
        ChronoSort::Rules::MeasureCpuBalance(rules, ChronoSort::Rules::LoadAllCards());
        return 0;
    }
#endif

    const std::string argument = (lpCmdLine != nullptr) ? lpCmdLine : "";

    // 「ChronoSort.exe warp」なら GPU を使わず WARP（ソフトウェア描画）で動かす（GPU が無い環境の確認用）
    Tsukino::Renderer::GraphicsDeviceOptions deviceOptions;
    if(argument.find("warp") != std::string::npos) {
        deviceOptions.preference = Tsukino::Renderer::GraphicsDevicePreference::ForceSoftware;
    }

    Tsukino::EngineIntegration::EngineIntegration engineIntegration;
    if(!engineIntegration.Initialize(1600, 900, "ChronoSort", Tsukino::Core::Window::WindowStyle::Default, deviceOptions)) {
        Tsukino::Core::Log::Error("Failed to initialize EngineIntegration.");
        MessageBoxW(nullptr, L"描画の初期化に失敗しました。グラフィックスドライバーを更新してから、もう一度起動してください。", L"ChronoSort", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }

    // ゲームのコンポーネントを Prefab から作れるようにする（最初のシーンより前に）
    ChronoSort::Game::Prefabs::RegisterComponents(*engineIntegration.GetContext().prefabFactory);

    // シーンをまたいで共有する音と設定。前回の設定（音量など）は、最初の音が鳴る前にここで反映される。
    // エンジンより後に作るので、エンジンより先に壊れる（音の後片付けがエンジンの片付けより前に済む）
    ChronoSort::Game::GameServices services(engineIntegration.GetContext());

    // 画質を決める（カード面の読み込みの規則も含むので、読込画面より先に）。
    // 「quality=high」などを付けると、「自動」で始める段階を指定できる（自動の引き下げの確認用）
    std::optional<Tsukino::Renderer::RenderQualityLevel> autoStart;
    if(argument.find("quality=high") != std::string::npos) {
        autoStart = Tsukino::Renderer::RenderQualityLevel::High;
    } else if(argument.find("quality=medium") != std::string::npos) {
        autoStart = Tsukino::Renderer::RenderQualityLevel::Medium;
    } else if(argument.find("quality=low") != std::string::npos) {
        autoStart = Tsukino::Renderer::RenderQualityLevel::Low;
    } else if(argument.find("quality=minimum") != std::string::npos) {
        autoStart = Tsukino::Renderer::RenderQualityLevel::Minimum;
    }
    const int choice = std::clamp(services.Settings().graphicsQuality, 0, static_cast<int>(ChronoSort::Game::GraphicsQualityChoice::Low));
    services.Graphics().Start(static_cast<ChronoSort::Game::GraphicsQualityChoice>(choice), autoStart);

    Tsukino::EngineIntegration::EngineAPI engineAPI(engineIntegration.GetContext());
    ChronoSort::Scene::Router::RegisterScenes(engineAPI);
    // 最初に読込画面でアセットをまとめて読み、その後の画面切替や操作で止まらないようにする。
    // 「ChronoSort.exe solo」「ChronoSort.exe versus」なら、読み終えたあとタイトルを飛ばして対局画面へ（確認用）
    // 続けて japan / world を書くと、出題をその範囲に絞る（例：「ChronoSort.exe solo japan」）。
    // easy / normal を書くと、出題をその難易度に絞る（例：「ChronoSort.exe solo easy japan」）
    auto next = [argument]() -> std::unique_ptr<Tsukino::EngineIntegration::GameSceneBase> {
        namespace Router = ChronoSort::Scene::Router;
        using ChronoSort::Rules::CardDifficulty;
        using ChronoSort::Rules::CardRange;
        using ChronoSort::Rules::CpuLevel;
        CardRange range = CardRange::All;
        if(argument.find("japan") != std::string::npos) {
            range = CardRange::Japan;
        } else if(argument.find("world") != std::string::npos) {
            range = CardRange::World;
        }
        CardDifficulty difficulty = CardDifficulty::Hard;
        if(argument.find("easy") != std::string::npos) {
            difficulty = CardDifficulty::Easy;
        } else if(argument.find("normal") != std::string::npos) {
            difficulty = CardDifficulty::Normal;
        }
        if(argument.find("solo") != std::string::npos) {
            return std::make_unique<ChronoSort::Scene::GameScene>(Router::Solo(range, difficulty));
        }
        if(argument.find("versus") != std::string::npos) {
            return std::make_unique<ChronoSort::Scene::GameScene>(Router::Versus(range, difficulty, {CpuLevel::Easy, CpuLevel::Normal}));
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
        services.Graphics().Tick(deltaTime);
        // ウィンドウドラッグ等で1フレームが極端に長くなったときの暴走防止
        deltaTime = std::min(deltaTime, 1.0f / 15.0f);

        engineAPI.Update(deltaTime);
        engineAPI.Render();
    }

    ChronoSort::Game::FrameMonitor::Report();
    return 0;
}
