//----------------------------------------------------------------------------
//! @file   PauseSheet.hpp
//! @brief  対局の一時停止の紙
//! @detail 「続ける」「設定」「タイトルへ」の3枚の札を貼った紙です。選ばれた行動は Poll で受け取ります。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/UI/PaperUI.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>

namespace ChronoSort::UI {

    //! 一時停止の紙で選ばれた行動です。
    enum class PauseChoice {
        None,       // まだ選ばれていない
        Resume,     // 続ける
        Options,    // 設定
        Title,      // タイトルへ
    };

    //! 対局の一時停止の紙です。
    class PauseSheet {
    public:

        //! 作り先のシーンとアセット管理を受け取ります。
        //! @param  [in] scene        UI を作るシーン。
        //! @param  [in] assetManager アセット管理。
        PauseSheet(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager);

        //! 紙を出します（出ていれば何もしません）。
        void Open();

        //! 紙を引っ込めます。
        void Close();

        //! 紙が出ているかを返します。
        //! @return 出ていれば true。
        bool IsOpen() const { return m_sheet != entt::null; }

        //! 選ばれた行動を返します。
        //! @return 選ばれた行動。
        PauseChoice Poll();

    private:
        PaperUI              m_ui;                      // 紙片の部品
        Tsukino::ECS::Entity m_sheet   = entt::null;    // 一時停止の紙
        Tsukino::ECS::Entity m_resume  = entt::null;    // 続ける
        Tsukino::ECS::Entity m_options = entt::null;    // 設定
        Tsukino::ECS::Entity m_title   = entt::null;    // タイトルへ
    };

}    // namespace ChronoSort::UI
