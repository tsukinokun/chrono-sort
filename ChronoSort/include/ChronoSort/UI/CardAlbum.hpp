//----------------------------------------------------------------------------
//! @file   CardAlbum.hpp
//! @brief  対局の後に、出たカードを見る紙
//! @detail 結果の羊皮紙の上に大きな紙を重ね、この対局で山札から出たカード（場・外れ・手札に残ったもの）を年代順に並べます。
//!         頁は「次の頁」「前の頁」の札・左右キー・ホイールでめくります。外したカードには朱印、手札のカードには札を添えます。
//!         カードを押すと、その1枚を大きく出し、題名・年代・分野・絵の出典を書きます。
//!         紙と札の置き方は Data/UI/CardAlbum.json にあります。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/Match.hpp>
#include <ChronoSort/UI/PaperLayout.hpp>
#include <ChronoSort/UI/PaperUI.hpp>
#include <ChronoSort/Util/Credits.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <vector>

namespace Tsukino::Input {
    class InputSystem;
}    // namespace Tsukino::Input

namespace ChronoSort::UI {

    //! 対局の後に、出たカードを見る紙です。
    class CardAlbum {
    public:

        //! 作り先のシーン・アセット管理・見せる対局を受け取ります。
        //! @param  [in] scene        UI を作るシーン。
        //! @param  [in] assetManager アセット管理。
        //! @param  [in] match        見せる対局（終わった後の手札・場・捨て札から行方を決める）。
        CardAlbum(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager, const Rules::Match& match);

        //! 紙を出します（出ていれば何もしません）。
        void Open();

        //! 1段戻ります。拡大中なら一覧へ、一覧なら紙を引っ込めます。
        void Back();

        //! 紙が出ているかを返します。
        //! @return 出ていれば true。
        bool IsOpen() const { return m_sheet != entt::null; }

        //! 札・カードのクリックと、頁送りのキー・ホイールを受けます（Esc は受けない。GameScene が Back を呼ぶ）。
        //! @param  [in] input 入力。
        void Update(const Tsukino::Input::InputSystem& input);

    private:

        //! 指定した頁を出します。
        //! @param  [in] page      頁（0 始まり）。
        //! @param  [in] direction めくる向き（1 で次、-1 で前、0 で滑り込ませない）。
        void ShowPage(int page, int direction);

        //! カードを1枚大きく出します。
        //! @param  [in] index m_cards の何枚目か。
        void ShowZoom(size_t index);

        //! 大きく出したカードを引っ込めます。
        void CloseZoom();

        PaperUI                        m_ui;        // 紙片の部品
        const Rules::Match&            m_match;     // 見せる対局
        PaperLayout                    m_layout;    // 紙と札の置き方（Data/UI/CardAlbum.json）
        Util::Credits                  m_credits;   // 絵の出典（初めて開いたときに読む）
        std::vector<Rules::CardData>   m_cards;     // 出たカード（年代順）
        std::vector<Tsukino::ECS::Entity> m_pageCards;    // 今の頁のカード（m_cards の first + i 番）

        Tsukino::ECS::Entity m_sheet     = entt::null;    // 一覧の紙
        Tsukino::ECS::Entity m_page      = entt::null;    // 今の頁の入れ物
        Tsukino::ECS::Entity m_pageNo    = entt::null;    // 頁番号
        Tsukino::ECS::Entity m_prev      = entt::null;    // 前の頁
        Tsukino::ECS::Entity m_next      = entt::null;    // 次の頁
        Tsukino::ECS::Entity m_close     = entt::null;    // 閉じる
        Tsukino::ECS::Entity m_catcher   = entt::null;    // 拡大中、紙の外のクリックを受ける透明な板
        Tsukino::ECS::Entity m_zoom      = entt::null;    // 拡大の紙
        Tsukino::ECS::Entity m_zoomBack  = entt::null;    // 一覧に戻る
        int                  m_current   = 0;             // 今の頁
        int                  m_pageCount = 1;             // 頁の数
        bool                 m_loaded    = false;         // 出典を読んだか
    };

}    // namespace ChronoSort::UI
