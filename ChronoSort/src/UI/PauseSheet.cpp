//----------------------------------------------------------------------------
//! @file   PauseSheet.cpp
//! @brief  対局の一時停止の紙の実装
//----------------------------------------------------------------------------
#include <ChronoSort/UI/PauseSheet.hpp>

#include <string>

namespace ChronoSort::UI {

    namespace {
        constexpr int kLayerPause = 85;    // 一時停止の紙の描画順（結果の紙より手前）
    }    // namespace

    //----------------------------------------------------------------------------
    //! 作り先のシーンとアセット管理を受け取ります。
    //----------------------------------------------------------------------------
    PauseSheet::PauseSheet(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager)
        : m_ui(scene, assetManager) {
    }

    //----------------------------------------------------------------------------
    //! 紙を出します。
    //----------------------------------------------------------------------------
    void PauseSheet::Open() {
        if(m_sheet != entt::null) {
            return;
        }
        PaperSpec sheet;
        sheet.texture      = "scrap_square.png";
        sheet.center       = hlslpp::float2(kReferenceWidth * 0.5f, kReferenceHeight * 0.5f - 10.0f);
        sheet.size         = hlslpp::float2(360.0f, 380.0f);
        sheet.angleDegrees = -2.5f;
        sheet.swayDegrees  = 0.4f;
        sheet.sortOrder    = kLayerPause;
        m_sheet            = m_ui.Paper(sheet);
        m_ui.SlideIn(m_sheet, hlslpp::float2(0.0f, -560.0f), 0.0f, 0.45f);

        const int layer = kLayerPause + 1;
        m_ui.Text(L"一時停止", hlslpp::float2(0.0f, -118.0f), 0.95f, kInkColor, layer, m_sheet);

        // 札は高さも傾きも少しずつずらす
        auto button = [&](const std::wstring& text, const hlslpp::float2& at, float angle) {
            PaperSpec spec;
            spec.texture      = "scrap_note.png";
            spec.center       = at;
            spec.size         = hlslpp::float2(210.0f, 64.0f);
            spec.angleDegrees = angle;
            spec.sortOrder    = layer + 1;
            spec.parent       = m_sheet;
            const auto paper  = m_ui.Paper(spec);
            m_ui.Text(text, hlslpp::float2(6.0f, 2.0f), 0.72f, kInkColor, layer + 2, paper);
            m_ui.MakeButton(paper);
            return paper;
        };
        m_resume  = button(L"続ける", hlslpp::float2(-6.0f, -38.0f), 2.0f);
        m_options = button(L"設定", hlslpp::float2(10.0f, 40.0f), -3.0f);
        m_title   = button(L"タイトルへ", hlslpp::float2(-4.0f, 118.0f), 1.5f);
    }

    //----------------------------------------------------------------------------
    //! 紙を引っ込めます。
    //----------------------------------------------------------------------------
    void PauseSheet::Close() {
        if(m_sheet == entt::null) {
            return;
        }
        // 滑り出してから片付ける（滑り出しの間に札を押されないよう、先に参照を外す）
        m_ui.SlideOut(m_sheet, hlslpp::float2(0.0f, -560.0f));
        m_ui.DestroyAfter(m_sheet, 0.4f);
        m_sheet   = entt::null;
        m_resume  = entt::null;
        m_options = entt::null;
        m_title   = entt::null;
    }

    //----------------------------------------------------------------------------
    //! 選ばれた行動を返します。
    //----------------------------------------------------------------------------
    PauseChoice PauseSheet::Poll() {
        if(m_sheet == entt::null) {
            return PauseChoice::None;
        }
        if(m_ui.Clicked(m_resume)) {
            return PauseChoice::Resume;
        }
        if(m_ui.Clicked(m_options)) {
            return PauseChoice::Options;
        }
        if(m_ui.Clicked(m_title)) {
            return PauseChoice::Title;
        }
        return PauseChoice::None;
    }

}    // namespace ChronoSort::UI
