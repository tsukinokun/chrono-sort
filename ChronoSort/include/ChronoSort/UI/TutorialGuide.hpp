//----------------------------------------------------------------------------
//! @file   TutorialGuide.hpp
//! @brief  慣れるまでの対局で出す、遊び方の説明と手本
//! @detail MatchSession::tutorial の対局だけ動きます（カードを正しく置けた対局が InputTuning::tutorialMatches 回になるまで）。
//!         最初の人間の手番で、画面を暗くして中央に遊び方の紙を出し、「わかった」を押すまで対局を止めます。
//!         閉じると紙は左下へ縮んで飛び（そこに DeskHud が小さな説明を残す）、TutorialDismissedEvent を出します。
//!         そのあと、半透明のカードと指先の光が、手札から場の列の隙間へ運ぶ手本を繰り返します。
//!         カードを掴むか選ぶと止め、放置されたら（IdleNudgeEvent）また見せます。
//!         手本の行き先は正解を教えないよう、列の左端と右端の隙間を交互にします。
//!         紙と手本の置き方は Data/UI/Tutorial.json、時間は Data/Tuning/Input.json にあります。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/UI/PaperLayout.hpp>
#include <ChronoSort/UI/PaperUI.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Core/ECS/Event/ScopedConnection.hpp>

#include <vector>

namespace Tsukino::Input {
    class InputSystem;
}    // namespace Tsukino::Input

namespace ChronoSort::UI {

    //! 慣れるまでの対局で出す、遊び方の説明と手本です。
    class TutorialGuide {
    public:

        //! 作り先のシーンとアセット管理を受け取り、出来事の購読を始めます。
        //! @param  [in] scene        UI を作るシーン（このシーンの EventBus を購読する）。
        //! @param  [in] assetManager アセット管理。
        TutorialGuide(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager);

        //! 遊び方の紙が出ているかを返します（出ている間は対局を止める）。
        //! @return 出ていれば true。
        bool IsOpen() const { return m_sheet != entt::null; }

        //! 遊び方の紙を閉じます（「わかった」と同じ。Esc で GameScene が呼ぶ）。
        void Dismiss();

        //! 「わかった」の札と Enter・Space を受け、手本を動かします。
        //! @param  [in] input     入力。
        //! @param  [in] deltaTime 経過時間（秒）。
        void Update(const Tsukino::Input::InputSystem& input, float deltaTime);

    private:

        //! 遊び方の紙を出します。
        void Open();

        //! 手本を始めます（出ていれば何もしない）。
        void StartDemo();

        //! 手本を片付けます。
        void StopDemo();

        //! 手本の次の1回を用意します（運ぶ手札と行き先の隙間を決める）。
        //! @return 用意できたら true（手札が無いなど、運べなければ false）。
        bool PrepareDemoCycle();

        PaperUI                                     m_ui;             // 紙片の部品
        PaperLayout                                 m_layout;         // 紙と手本の置き方（Data/UI/Tutorial.json）
        std::vector<Tsukino::ECS::ScopedConnection> m_connections;    // 出来事の購読

        Tsukino::ECS::Entity m_dim   = entt::null;    // 暗幕
        Tsukino::ECS::Entity m_sheet = entt::null;    // 遊び方の紙
        Tsukino::ECS::Entity m_ok    = entt::null;    // わかった
        float                m_okGuard = 0.0f;        // 「わかった」を受け付けるまでの残り時間（秒）
        bool                 m_shown   = false;       // この対局で遊び方の紙をもう出したか

        Tsukino::ECS::Entity m_from    = entt::null;    // 手本の出発点（手札のカードに追従する見えない点）
        Tsukino::ECS::Entity m_to      = entt::null;    // 手本の行き先（隙間に貼り付く見えない点）
        Tsukino::ECS::Entity m_card    = entt::null;    // 半透明のカード
        Tsukino::ECS::Entity m_pointer = entt::null;    // 指先の光
        Tsukino::ECS::Entity m_label   = entt::null;    // 手本に添える札
        float                m_time    = 0.0f;          // 今の1回の経過時間（秒）
        int                  m_cycle   = 0;             // 何回目の手本か（行き先の左右を交互にする）
        int                  m_settle  = 0;             // 見えない点が画面へ写されるまで、手本を出さずに待つフレーム数
        bool                 m_dismissed = false;       // 遊び方の紙を閉じたか（手本はそれから）
    };

}    // namespace ChronoSort::UI
