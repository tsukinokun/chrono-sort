//----------------------------------------------------------------------------
//! @file   OptionsSheet.hpp
//! @brief  設定の紙（音量・演出の速さ・画面の揺れ）
//! @detail タイトルと対局の両方から開く、机に置いた1枚の紙です。
//!         音量は墨の線を指で引いて決め（線の先に封蝋のつまみ。つかむのはエンジンの UISliderComponent）、
//!         動かしている間はその音で試し鳴りします。演出の速さと画面の揺れは選択肢の札（UI::ChoiceTag）です。
//!         値は設定（Game::Services().Settings()）を直接書き換え、音量はその場で反映し、閉じたときに保存します。
//!         紙と札の置き方は Data/UI/Options.json に書きます。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/UI/PaperLayout.hpp>
#include <ChronoSort/UI/PaperUI.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <string>
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
        //! @param  [in] context   エンジンのコンテキスト（入力・アセット）。
        //! @param  [in] sortOrder 紙の描画順（その画面の他の UI より手前にする）。
        OptionsSheet(Tsukino::ECS::Scene& scene, Tsukino::EngineIntegration::EngineContext& context, int sortOrder);

        //! 紙を開きます（開いていれば何もしません）。
        void Open();

        //! 紙を閉じ、設定を保存します。
        void Close();

        //! 開いているかを返します。
        //! @return 開いていれば true。
        bool IsOpen() const { return m_sheet != entt::null; }

        //! 毎フレームの入力と見た目の更新を行います。
        //! @param  [in] deltaTime 経過時間（秒）。
        //! @return このフレームに閉じたら true。
        bool Update(float deltaTime);

    private:

        //! 音量のスライダー1本です。
        struct Slider {
            float*               value   = nullptr;       // 書き換える設定の値（0〜1）
            bool                 preview = true;          // 動かしたときに試し鳴りするか（BGM は流れている曲がそのまま試し鳴りになるので false）
            float                shown   = 0.0f;          // 線の今の長さ（値をなめらかに追う）
            Tsukino::ECS::Entity fill    = entt::null;    // 墨の線
            Tsukino::ECS::Entity knob    = entt::null;    // 封蝋のつまみ
            Tsukino::ECS::Entity hit     = entt::null;    // 当たり判定（UISliderComponent を持つ）
            Tsukino::ECS::Entity text    = entt::null;    // 値の数字
        };

        //! 札2枚の切り替え1つです。
        struct Toggle {
            bool*                value = nullptr;       // 書き換える設定の値（選んだ札の値を入れる）
            Tsukino::ECS::Entity group = entt::null;    // 札の組
        };

        //! スライダーを1本作ります。
        //! @param  [in] label   名前。
        //! @param  [in] row     置き方の名前（Options.json。center.y が行の高さ）。
        //! @param  [in] value   書き換える設定の値。
        //! @param  [in] preview 動かしたときに試し鳴りするか。
        void AddSlider(const std::wstring& label, const std::string& row, float* value, bool preview);

        //! 札2枚の切り替えを1つ作ります。
        //! @param  [in] label 名前。
        //! @param  [in] row   置き方の名前（Options.json。center.y が行の高さ）。
        //! @param  [in] first 1枚目の札の文字と、選んだときの値。
        //! @param  [in] second 2枚目の札の文字と、選んだときの値。
        //! @param  [in] value 書き換える設定の値。
        void AddToggle(const std::wstring& label, const std::string& row, const std::pair<std::wstring, bool>& first,
                       const std::pair<std::wstring, bool>& second, bool* value);

        Tsukino::ECS::Scene&                       m_scene;           // UI を作るシーン
        Tsukino::EngineIntegration::EngineContext& m_context;         // 入力（Esc で閉じる）
        PaperUI                                    m_ui;              // 紙片の部品
        PaperLayout                                m_layout;          // 置き方（Data/UI/Options.json）
        int                                        m_sortOrder;       // 紙の描画順
        Tsukino::ECS::Entity                       m_sheet = entt::null;    // 紙
        Tsukino::ECS::Entity                       m_close = entt::null;    // 閉じる
        std::vector<Slider>                        m_sliders;         // 音量（全体・BGM・効果音）
        std::vector<Toggle>                        m_toggles;         // 演出の速さ・画面の揺れ
        float                                      m_previewTimer = 0.0f;    // 試し鳴りの間隔
    };

}    // namespace ChronoSort::UI
