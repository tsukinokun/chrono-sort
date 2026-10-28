//----------------------------------------------------------------------------
//! @file   OptionsSheet.hpp
//! @brief  設定の紙（音量・演出の速さ・画面の揺れ）
//! @detail タイトルと対局の両方から開く、机に置いた1枚の紙です。
//!         音量は墨の線を指で引いて決め（線の先に封蝋のつまみ）、動かしている間はその音で試し鳴りします。
//!         値は Game::CurrentOptions() を直接書き換え、音量はその場で反映し、閉じたときに保存します。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/UI/PaperUI.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <vector>

namespace Tsukino::ECS {
    class Scene;
}    // namespace Tsukino::ECS

namespace Tsukino::EngineIntegration {
    struct EngineContext;
}    // namespace Tsukino::EngineIntegration

namespace ChronoSort::UI {

    //! 設定の紙です。
    class OptionsSheet {
    public:

        //! 作り先のシーンと描画順を受け取ります。
        //! @param  [in] scene     UI を作るシーン。
        //! @param  [in] context   エンジンのコンテキスト（入力・音・アセット）。
        //! @param  [in] sortOrder 紙の描画順（その画面の他の UI より手前にする）。
        OptionsSheet(Tsukino::ECS::Scene& scene, Tsukino::EngineIntegration::EngineContext& context, int sortOrder);

        //! 紙を開きます（開いていれば何もしません）。
        void Open();

        //! 紙を閉じ、設定を保存します。
        void Close();

        //! 開いているかを返します。
        //! @return 開いていれば true。
        bool IsOpen() const { return m_sheet != entt::null && !m_closing; }

        //! 毎フレームの入力と見た目の更新を行います。
        //! @param  [in] deltaTime 経過時間（秒）。
        //! @return このフレームに閉じたら true。
        bool Update(float deltaTime);

    private:

        //! 音量のスライダー1本です。
        struct Slider {
            float*               value = nullptr;          // 書き換える設定の値（0〜1）
            float                shown = 0.0f;             // 線の今の長さ（値をなめらかに追う）
            Tsukino::ECS::Entity track = entt::null;       // 線の始点（ローカル座標の原点）
            Tsukino::ECS::Entity fill  = entt::null;       // 墨の線
            Tsukino::ECS::Entity knob  = entt::null;       // 封蝋のつまみ
            Tsukino::ECS::Entity hit   = entt::null;       // 当たり判定
            Tsukino::ECS::Entity text  = entt::null;       // 値の数字
        };

        //! 札2枚の切り替え1つです。
        struct Toggle {
            bool*                             value = nullptr;    // 書き換える設定の値（2枚目を選ぶと true。画面の揺れだけは1枚目が true）
            std::vector<Tsukino::ECS::Entity> tags;               // 札
            std::vector<Tsukino::ECS::Entity> texts;              // 札の文字
        };

        //! スライダーを1本作ります。
        //! @param  [in] label 名前。
        //! @param  [in] y     紙の中心からの高さ。
        //! @param  [in] value 書き換える設定の値。
        void AddSlider(const std::wstring& label, float y, float* value);

        //! 札2枚の切り替えを1つ作ります。
        //! @param  [in] label   名前。
        //! @param  [in] y       紙の中心からの高さ。
        //! @param  [in] off     1枚目（false）の札の文字。
        //! @param  [in] on      2枚目（true）の札の文字。
        //! @param  [in] value   書き換える設定の値。
        void AddToggle(const std::wstring& label, float y, const std::wstring& off, const std::wstring& on, bool* value);

        //! 切り替えの札の見た目を、今の値に合わせます。
        //! @param  [in] toggle 切り替え。
        //! @param  [in] punch  選んだ札を弾ませるなら true。
        void RefreshToggle(Toggle& toggle, bool punch);

        //! マウスの位置を、スライダーの線のローカル座標の X にします。
        //! @param  [in] slider スライダー。
        //! @return 線の始点からの距離（UI の座標）。
        float MouseAlong(const Slider& slider);

        //! スライダーの値を音量に反映し、試し鳴りします。
        //! @param  [in] index スライダーの番号。
        void OnSliderChanged(size_t index);

        Tsukino::ECS::Scene&                        m_scene;          // UI を作るシーン
        Tsukino::EngineIntegration::EngineContext&  m_context;        // 入力・音
        PaperUI                                     m_ui;             // 紙片の部品
        int                                         m_sortOrder;      // 紙の描画順
        Tsukino::ECS::Entity                        m_sheet = entt::null;    // 紙
        Tsukino::ECS::Entity                        m_close = entt::null;    // 閉じる
        std::vector<Slider>                         m_sliders;        // 音量（全体・BGM・効果音）
        std::vector<Toggle>                         m_toggles;        // 演出の速さ・画面の揺れ
        int                                         m_dragging = -1;  // ドラッグ中のスライダー
        float                                       m_previewTimer = 0.0f;    // 試し鳴りの間隔
        bool                                        m_closing      = false;   // 滑り出している最中か
        float                                       m_closeTimer   = 0.0f;    // 滑り出し終わるまでの時間
    };

}    // namespace ChronoSort::UI
