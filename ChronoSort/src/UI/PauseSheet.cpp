//----------------------------------------------------------------------------
//! @file   PauseSheet.cpp
//! @brief  対局の一時停止の紙の実装
//----------------------------------------------------------------------------
#include <ChronoSort/UI/PauseSheet.hpp>

#include <ChronoSort/UI/Widgets.hpp>

#include <string>

namespace ChronoSort::UI {

    //----------------------------------------------------------------------------
    //! 作り先のシーンとアセット管理を受け取ります。
    //----------------------------------------------------------------------------
    PauseSheet::PauseSheet(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager)
        : m_ui(scene, assetManager)
        , m_layout("ChronoSort/Assets/Data/UI/GameHud.json") {
    }

    //----------------------------------------------------------------------------
    //! 紙を出します。
    //----------------------------------------------------------------------------
    void PauseSheet::Open() {
        if(m_sheet != entt::null) {
            return;
        }
        const PaperSpec sheet = m_layout.Spec("pauseSheet");
        m_sheet               = m_ui.Paper(sheet);
        m_ui.SlideIn(m_sheet, hlslpp::float2(0.0f, -560.0f), 0.0f, 0.45f);
        m_ui.Text(L"一時停止", hlslpp::float2(0.0f, -118.0f), 0.95f, kInkColor, sheet.sortOrder + 1, m_sheet);

        // 札は高さも傾きも少しずつずらす（Data/UI/GameHud.json）
        auto button = [&](const char* name, const std::wstring& text) {
            PaperSpec spec = m_layout.Spec(name);
            spec.parent    = m_sheet;
            return NoteButton(m_ui, spec, text, 0.72f);
        };
        m_resume  = button("pauseResume", L"続ける");
        m_options = button("pauseOptions", L"設定");
        m_title   = button("pauseTitle", L"タイトルへ");
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
