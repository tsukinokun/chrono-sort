//----------------------------------------------------------------------------
//! @file   GraphicsQuality.cpp
//! @brief  画質の決定と、重いときの自動の引き下げの実装
//----------------------------------------------------------------------------
#include <ChronoSort/Game/GraphicsQuality.hpp>

#include <ChronoSort/Game/GameServices.hpp>
#include <ChronoSort/Util/AssetPaths.hpp>

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Window.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Renderer/Renderer.hpp>

#include <algorithm>
#include <string>

namespace ChronoSort::Game {

    namespace {
        using Tsukino::Renderer::RenderQualityLevel;

        //----------------------------------------------------------------------------
        //! 設定の値を段階へ変えます。
        //! @param  [in] choice 設定の画質（Auto 以外）。
        //! @return 段階。
        //----------------------------------------------------------------------------
        RenderQualityLevel ToLevel(GraphicsQualityChoice choice) {
            switch(choice) {
                case GraphicsQualityChoice::Medium: return RenderQualityLevel::Medium;
                case GraphicsQualityChoice::Low:    return RenderQualityLevel::Low;
                default:                            return RenderQualityLevel::High;
            }
        }

        //----------------------------------------------------------------------------
        //! 1段下の段階を返します。
        //! @param  [in] level 段階（Minimum 以外）。
        //! @return 1段下の段階。
        //----------------------------------------------------------------------------
        RenderQualityLevel NextLower(RenderQualityLevel level) {
            return static_cast<RenderQualityLevel>(static_cast<int>(level) + 1);
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! エンジンを受け取ります。
    //----------------------------------------------------------------------------
    GraphicsQuality::GraphicsQuality(Tsukino::EngineIntegration::EngineContext& context)
        : m_context(context) {
    }

    //----------------------------------------------------------------------------
    //! 起動時の画質を決めて適用します。
    //----------------------------------------------------------------------------
    void GraphicsQuality::Start(GraphicsQualityChoice choice, std::optional<RenderQualityLevel> autoStart) {
        // カード面は枚数が多いので、ブロック圧縮とミップマップを前もって作っておく（読み込みの規則は最初の読み込みより前に）
        Tsukino::Asset::TextureImportRule cardRule;
        cardRule.compressedFormat = DXGI_FORMAT_BC7_UNORM;
        cardRule.generateMips     = true;
        cardRule.allowMipSkip     = true;
        m_context.assetManager->SetTextureImportRule(std::string(Util::AssetPaths::kCardTextureDir) + "/", cardRule);

        m_autoStartLevel = autoStart.value_or(Tsukino::Renderer::RecommendRenderQuality(m_context.renderer->GetAdapterInfo()));
        SetChoice(choice);

        // 読み込み済みのテクスチャは小さくできないので、切り捨ての段数はここで1回だけ決める
        m_context.assetManager->SetTextureMipSkip(Tsukino::Renderer::GetRenderQualityPreset(m_level).textureMipSkip);
    }

    //----------------------------------------------------------------------------
    //! 設定の画質を変えます。
    //----------------------------------------------------------------------------
    void GraphicsQuality::SetChoice(GraphicsQualityChoice choice) {
        m_isAuto = (choice == GraphicsQualityChoice::Auto);
        ApplyLevel(m_isAuto ? m_autoStartLevel : ToLevel(choice));
        m_governor.Reset();
    }

    //----------------------------------------------------------------------------
    //! フレーム間隔の測定を止めるかを設定します。
    //----------------------------------------------------------------------------
    void GraphicsQuality::SetPaused(bool paused) {
        m_paused = paused;
        m_governor.Reset();
    }

    //----------------------------------------------------------------------------
    //! 1フレーム分の間隔を与えます。
    //----------------------------------------------------------------------------
    void GraphicsQuality::Tick(float deltaSeconds) {
        // 裏に回っているあいだは Windows に絞られて遅くなるので、描画の重さとして数えない
        if(!m_isAuto || m_paused || m_level == RenderQualityLevel::Minimum || !m_context.window->IsFocused()) {
            return;
        }

        const GraphicsTuning&                         tuning = Services().Tunings().Graphics();
        Tsukino::Renderer::RenderQualityGovernorSettings settings;
        settings.targetFrameSeconds = 1.0f / std::max(tuning.targetFps, 1.0f);
        settings.sustainSeconds     = tuning.sustainSeconds;
        settings.cooldownSeconds    = tuning.cooldownSeconds;
        m_governor.SetSettings(settings);

        if(m_governor.Tick(deltaSeconds)) {
            Tsukino::Core::Log::Warn("[Graphics] Frames stayed over budget; lowering the quality one step.");
            ApplyLevel(NextLower(m_level));
        }
    }

    //----------------------------------------------------------------------------
    //! 段階を描画へ適用します。
    //----------------------------------------------------------------------------
    void GraphicsQuality::ApplyLevel(RenderQualityLevel level) {
        m_level = level;
        m_context.renderer->SetQuality(Tsukino::Renderer::GetRenderQualityPreset(level));
        Tsukino::Core::Log::Info(std::string("[Graphics] Quality: ") + Tsukino::Renderer::GetRenderQualityName(level) + (m_isAuto ? " (auto)" : " (fixed)"));
    }

}    // namespace ChronoSort::Game
