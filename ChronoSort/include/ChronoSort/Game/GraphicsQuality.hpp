//----------------------------------------------------------------------------
//! @file   GraphicsQuality.hpp
//! @brief  画質の決定と、重いときの自動の引き下げ
//! @detail 起動時に動く環境（GPU の種類とメモリ）から画質の段階を選び、「自動」のあいだは
//!         フレーム間隔を見て重ければ1段ずつ下げます。設定の「高・中・低」を選ぶと固定します。
//!         GameServices が持ちます。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Game/Options.hpp>

#include <Tsukino/Core/IO/CerealOptional.hpp>
#include <Tsukino/Renderer/RenderQuality.hpp>
#include <Tsukino/Renderer/RenderQualityGovernor.hpp>

#include <optional>

namespace Tsukino::EngineIntegration {
    struct EngineContext;
}    // namespace Tsukino::EngineIntegration

namespace ChronoSort::Game {

    //! 自動の引き下げの調整値です（Graphics.json）。
    struct GraphicsTuning {
        float targetFps       = 30.0f;    // これを平均で割り続けたら下げる（フレーム毎秒）
        float sustainSeconds  = 3.0f;     // 割り続けてから下げるまでの時間（秒）
        float cooldownSeconds = 6.0f;     // 下げたあと次の判断を始めるまでの時間（秒）
    };

    template <class Archive>
    void load(Archive& archive, GraphicsTuning& t) {
        Tsukino::IO::LoadOptional(archive, "targetFps", t.targetFps);
        Tsukino::IO::LoadOptional(archive, "sustainSeconds", t.sustainSeconds);
        Tsukino::IO::LoadOptional(archive, "cooldownSeconds", t.cooldownSeconds);
    }

    //! 画質の決定と、重いときの自動の引き下げを行うものです。
    class GraphicsQuality {
    public:

        //! エンジンを受け取ります。
        //! @param  [in] context エンジンのコンテキスト。
        explicit GraphicsQuality(Tsukino::EngineIntegration::EngineContext& context);

        //! 起動時の画質を決めて適用します。最初のアセットを読む前に呼びます。
        //! @param  [in] choice    設定の画質。
        //! @param  [in] autoStart 「自動」で始める段階を指定するとき（確認用）。無ければ GPU から選ぶ。
        void Start(GraphicsQualityChoice choice, std::optional<Tsukino::Renderer::RenderQualityLevel> autoStart);

        //! 設定の画質を変えます（カードの細かさ以外はすぐ反映されます）。
        //! @param  [in] choice 設定の画質。
        void SetChoice(GraphicsQualityChoice choice);

        //! フレーム間隔の測定を止めるかを設定します（読み込み中は重くて当然なので止める）。
        //! @param  [in] paused 止めるなら true。
        void SetPaused(bool paused);

        //! 1フレーム分の間隔を与えます。「自動」のあいだ重ければ1段下げます。
        //! @param  [in] deltaSeconds 前のフレームからの実時間（秒）。
        void Tick(float deltaSeconds);

    private:

        //! 段階を描画へ適用します。
        //! @param  [in] level 段階。
        void ApplyLevel(Tsukino::Renderer::RenderQualityLevel level);

        Tsukino::EngineIntegration::EngineContext& m_context;                // エンジン
        Tsukino::Renderer::RenderQualityGovernor   m_governor;               // 下げるかどうかの判断
        Tsukino::Renderer::RenderQualityLevel      m_level   = Tsukino::Renderer::RenderQualityLevel::High;    // 今の段階
        Tsukino::Renderer::RenderQualityLevel      m_autoStartLevel = Tsukino::Renderer::RenderQualityLevel::High;    // 「自動」で始める段階
        bool                                       m_isAuto  = true;         // 「自動」か
        bool                                       m_paused  = false;        // 測定を止めているか
    };

}    // namespace ChronoSort::Game
