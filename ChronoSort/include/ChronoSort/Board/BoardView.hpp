//----------------------------------------------------------------------------
//! @file   BoardView.hpp
//! @brief  卓の上のカードの置き場所を変える窓口と、並びへの問い合わせ
//! @detail カードがどこに属しているかは CardPlacementComponent、卓全体の状態は BoardState
//!         （レジストリのコンテキスト）が持ち、毎フレームの並べ直しは BoardLayoutSystem が行います。
//!         ここは「列に入れる」「手札から出す」などの置き場所の変更（番号の詰め直しを含む）と、
//!         入力や演出が使う問い合わせ（スロットの位置・拾い判定）をまとめた窓口です。状態は持ちません。
//!         ルール上の正否はここでは扱わず、Rules::Match の結果に合わせて置き場所を変えるだけです。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Board/BoardLayout.hpp>
#include <ChronoSort/Board/BoardState.hpp>
#include <ChronoSort/Board/CardFactory.hpp>
#include <ChronoSort/Rules/CardData.hpp>

#include <Tsukino/BuiltIn/ECS/Camera/CameraUtility.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <hlsl++.h>

#include <optional>
#include <random>
#include <vector>

namespace ChronoSort::Board {

    //! 卓の上のカードの置き場所を変える窓口です。
    class BoardView {
    public:

        //! カードの生成器を受け取ります。
        //! @param  [in] factory カードの生成器。
        explicit BoardView(CardFactory& factory);

        //! 卓・挿入位置の目印を作り、卓の状態（BoardState）をレジストリに置きます。
        //! @param  [in] playerCount 参加人数（0 番が人間の手前、残りが奥の席）。
        void Build(size_t playerCount);

        //! 山札の位置にカードを生成します（配る・引くときの出発点。まだどこにも並べない）。
        //! @param  [in] card カードのデータ。
        //! @return 作成したエンティティ。
        Tsukino::ECS::Entity SpawnAtDeck(const Rules::CardData& card);

        //! 今の置き場所どおりの目標を、すぐに書き込みます（置き場所を変えた直後に姿勢を求めたいとき）。
        void ApplyLayoutNow();

        //! 場の列のスロットの中心（卓の上）を返します。今空けている隙間を考慮します。
        //! @param  [in] slot スロット番号。
        //! @return ワールド座標。
        hlslpp::float3 SlotPosition(size_t slot);

        //! 場の列のスロットに置いたカードの姿勢（カメラへ起こした向き）を返します。置く演出の着地先に使います。
        //! @param  [in]  slot     スロット番号。
        //! @param  [out] position 位置。
        //! @param  [out] rotation 向き。
        //! @param  [out] scale    拡大率。
        void SlotRestPose(size_t slot, hlslpp::float3& position, hlslpp::quaternion& rotation, float& scale);

        //! 場の列のスロットに置くカードの拡大率を返します（注目点に近いほど大きい）。
        //! @param  [in] slot スロット番号。
        //! @return 拡大率。
        float SlotScale(size_t slot);

        //! 場の列の注目点（カーソルや持っているカードの位置）を設定します。周りのカードを読める大きさに広げます。
        //! @param  [in] worldX 卓の上の X 座標。std::nullopt なら隙間を注目点にし、隙間も無ければ最後の注目点のままにする。
        void SetFocusX(std::optional<float> worldX) { State().focusX = worldX; }

        //! 場の列を横に流します（列が画面に収まらないときだけ動く）。
        //! @param  [in] amount 流す量（正で右側の続きを見る）。
        void ScrollBy(float amount);

        //! 指定したスロットが画面の中ほどに来るよう、場の列を流します。
        //! @param  [in] slot スロット番号。
        void ScrollToSlot(size_t slot);

        //! ワールドの X 座標から、最も近いスロットを返します。
        //! @param  [in] worldX ワールドの X 座標。
        //! @return スロット番号。
        size_t SlotFromWorldX(float worldX);

        //! レイが当たる手札のカードを探します（手前のカードを優先）。
        //! @param  [in]  player 手札の持ち主。
        //! @param  [in]  ray    マウスのレイ。
        //! @param  [out] uv     当たった位置のカード上の座標（0〜1）。
        //! @return 手札の番号。当たらなければ -1。
        int PickHandCard(size_t player, const Tsukino::BuiltIn::ECS::CameraUtility::Ray& ray, hlslpp::float2& uv);

        //! レイが当たる場のカードを探します。
        //! @param  [in] ray マウスのレイ。
        //! @return 場の何枚目か。当たらなければ -1。
        int PickRowCard(const Tsukino::BuiltIn::ECS::CameraUtility::Ray& ray);

        //! 場の列に隙間を空けます（挿入位置の予告・公開中のカードの置き場）。
        //! @param  [in] slot 隙間を空けるスロット。std::nullopt で閉じる。
        void SetGap(std::optional<size_t> slot) { State().gap = slot; }

        //! 隙間に置く目印の表示を切り替えます。
        //! @param  [in] visible 表示するなら true。
        //! @param  [in] warning 赤い目印（正しい位置を教えるとき）なら true。
        void SetMarker(bool visible, bool warning = false);

        //! 公開を終えたカードを場の列に入れます。
        //! @param  [in] slot   スロット番号。
        //! @param  [in] entity カード。
        void InsertIntoRow(size_t slot, Tsukino::ECS::Entity entity);

        //! カードを手札の最後に加えます。
        //! @param  [in] player 手札の持ち主。
        //! @param  [in] entity カード。
        void AddToHand(size_t player, Tsukino::ECS::Entity entity);

        //! 手札からカードを取り出します（どこにも並べない状態にし、後ろのカードを詰める）。
        //! @param  [in] player 手札の持ち主。
        //! @param  [in] index  手札の番号。
        //! @return 取り出したカード。
        Tsukino::ECS::Entity TakeFromHand(size_t player, size_t index);

        //! カードを捨て札の山に送ります（目標を捨て札の位置にして放物線で飛ばす）。
        //! @param  [in] entity カード。
        void SendToDiscard(Tsukino::ECS::Entity entity);

        //! 山札の残り枚数に合わせて、山の厚みを変えます。
        //! @param  [in] remaining 残り枚数。
        void SetDeckCount(size_t remaining);

        //! 手札の見せ方（ホバー・持ち上げなど）を返します。
        //! @param  [in] player プレイヤー番号。
        //! @return 手札の見せ方。
        HandState& Hand(size_t player) { return State().hands[player]; }

        //! 手札の枚数を返します。
        //! @param  [in] player プレイヤー番号。
        //! @return 枚数。
        size_t HandSize(size_t player);

        //! 手札の i 枚目のカードを返します。
        //! @param  [in] player プレイヤー番号。
        //! @param  [in] index  手札の番号。
        //! @return カード。
        Tsukino::ECS::Entity HandCard(size_t player, size_t index);

        //! 場の列のカードを返します。
        //! @return 場の列（年代順）。
        std::vector<Tsukino::ECS::Entity> Row();

        //! 手札のカードの、並びの中での姿勢を計算します（持ち上げ・ホバーを含む）。
        //! @param  [in]  player   プレイヤー番号。
        //! @param  [in]  index    手札の番号。
        //! @param  [out] position 位置。
        //! @param  [out] rotation 向き。
        //! @param  [out] scale    拡大率。
        void HandPose(size_t player, size_t index, hlslpp::float3& position, hlslpp::quaternion& rotation, float& scale);

        //! 山札の一番上の位置を返します。
        //! @return ワールド座標。
        hlslpp::float3 DeckTop();

        //! 場の列でカーソルが重なっているカードを設定します（持ち上げて年号を読みやすくする）。
        //! @param  [in] index 場の何枚目か。-1 で解除。
        void SetRowHovered(int index) { State().rowHovered = index; }

        //! 並べ直しの対象から外すカード（ドラッグ中・公開中など）を設定します。
        //! @param  [in] entity 外すカード。entt::null で解除。
        void SetHeld(Tsukino::ECS::Entity entity) { State().held = entity; }

    private:

        //! 卓の状態を返します。
        //! @return 卓の状態。
        BoardState& State();

        //! 場の列の枚数を返します。
        //! @return 枚数。
        size_t RowCount();

        CardFactory& m_factory;    // カードの生成器
        std::mt19937 m_rng;        // 捨て札・山札のずれ用の乱数
    };

    //! 対局のシーンのレジストリに置いた卓の窓口を返します（GameScene が置く）。
    //! @param  [in] registry レジストリ。
    //! @return 卓の窓口。
    inline BoardView& View(Tsukino::ECS::Registry& registry) {
        return registry.GetContext<BoardView>();
    }

}    // namespace ChronoSort::Board
