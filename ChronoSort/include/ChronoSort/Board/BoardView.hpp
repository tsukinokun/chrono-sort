//----------------------------------------------------------------------------
//! @file   BoardView.hpp
//! @brief  卓の上のカードの並べ方
//! @detail 場の列・各プレイヤーの手札・山札・捨て札のエンティティを持ち、
//!         毎フレーム「どこにあるべきか」を CardMotion の目標として書き込みます。
//!         ルール上の正否はここでは扱わず、Rules::Match の結果に合わせて並べ替えるだけです。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Board/CardFactory.hpp>
#include <ChronoSort/Rules/CardData.hpp>

#include <Tsukino/BuiltIn/ECS/Camera/CameraUtility.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <hlsl++.h>

#include <optional>
#include <random>
#include <vector>

namespace ChronoSort::Board {

    constexpr float kTableSurfaceY = 0.31f;     // 卓に寝かせたカードの高さ（板の厚みの半分 + 僅か）
    constexpr float kRowZ          = 70.0f;     // 場の列の奥行き
    constexpr float kDragHeight    = 45.0f;     // ドラッグ中のカードの高さ
    // 場の列のカードを、手前の辺を卓に付けたままカメラへ起こす角度（約26°）。
    // 寝かせたままだと約47°の見下ろしで縦が 0.73 倍に潰れ、題名と年号が読みにくい。
    // 真正面（約0.75rad）まで起こすと卓に置いた感じが消えるので、その手前で止める
    constexpr float kRowTilt       = 0.45f;

    //! 手札の見せ方です。
    enum class HandStyle {
        Fan,       // 手前の扇（人間）
        Seat,      // 卓の奥の席に並べる（CPU）
    };

    //! 1人分の手札の並び（カードと、そのうち特別扱いしているもの）です。
    struct HandView {
        std::vector<Tsukino::ECS::Entity> cards;                          // 手札（Rules::Player::hand と同じ順）
        HandStyle                         style      = HandStyle::Fan;    // 並べ方
        hlslpp::float3                    seat       = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 席の中心（Seat のとき）
        int                               hovered    = -1;    // カーソルが重なっている手札
        int                               lifted     = -1;    // 持ち上げている手札（選択中・CPU の思考中）
        hlslpp::float2                    hoverTilt  = hlslpp::float2(0.0f, 0.0f);    // カーソルの位置へ傾ける量
    };

    //! 卓の上のカードの並べ方です。
    class BoardView {
    public:

        //! カードの生成器を受け取ります。
        //! @param  [in] factory カードの生成器。
        explicit BoardView(CardFactory& factory);

        //! 卓・山札・挿入位置の目印を作ります。
        //! @param  [in] playerCount 参加人数（0 番が人間の手前、残りが奥の席）。
        void Build(size_t playerCount);

        //! 山札の位置にカードを生成します（配る・引くときの出発点）。
        //! @param  [in] card カードのデータ。
        //! @return 作成したエンティティ。
        Tsukino::ECS::Entity SpawnAtDeck(const Rules::CardData& card);

        //! 毎フレームの並べ直しを行います。
        //! @param  [in] deltaTime 経過時間（秒）。
        void UpdateLayout(float deltaTime);

        //! 場の列のスロットの中心（卓の上）を返します。今空けている隙間を考慮します。
        //! @param  [in] slot スロット番号。
        //! @return ワールド座標。
        hlslpp::float3 SlotPosition(size_t slot) const;

        //! 場の列のスロットに置いたカードの姿勢（カメラへ起こした向き）を返します。置く演出の着地先に使います。
        //! @param  [in]  slot     スロット番号。
        //! @param  [out] position 位置。
        //! @param  [out] rotation 向き。
        void SlotRestPose(size_t slot, hlslpp::float3& position, hlslpp::quaternion& rotation) const;

        //! ワールドの X 座標から、最も近いスロットを返します。
        //! @param  [in] worldX ワールドの X 座標。
        //! @return スロット番号。
        size_t SlotFromWorldX(float worldX) const;

        //! レイが当たる手札のカードを探します（手前のカードを優先）。
        //! @param  [in]  player 手札の持ち主。
        //! @param  [in]  ray    マウスのレイ。
        //! @param  [out] uv     当たった位置のカード上の座標（0〜1）。
        //! @return 手札の番号。当たらなければ -1。
        int PickHandCard(size_t player, const Tsukino::BuiltIn::ECS::CameraUtility::Ray& ray, hlslpp::float2& uv) const;

        //! レイが当たる場のカードを探します。
        //! @param  [in] ray マウスのレイ。
        //! @return 場の何枚目か。当たらなければ -1。
        int PickRowCard(const Tsukino::BuiltIn::ECS::CameraUtility::Ray& ray) const;

        //! 場の列に隙間を空けます（挿入位置の予告・公開中のカードの置き場）。
        //! @param  [in] slot 隙間を空けるスロット。std::nullopt で閉じる。
        void SetGap(std::optional<size_t> slot) { m_gap = slot; }

        //! 隙間に置く目印の表示を切り替えます。
        //! @param  [in] visible 表示するなら true。
        //! @param  [in] warning 赤い目印（正しい位置を教えるとき）なら true。
        void SetMarker(bool visible, bool warning = false);

        //! 公開を終えたカードを場の列に入れます。
        //! @param  [in] slot   スロット番号。
        //! @param  [in] entity カード。
        void InsertIntoRow(size_t slot, Tsukino::ECS::Entity entity);

        //! カードを捨て札の山に送ります（目標を捨て札の位置にして放物線で飛ばす）。
        //! @param  [in] entity カード。
        void SendToDiscard(Tsukino::ECS::Entity entity);

        //! 山札の残り枚数に合わせて、山の厚みを変えます。
        //! @param  [in] remaining 残り枚数。
        void SetDeckCount(size_t remaining);

        //! 手札の並びを返します。
        //! @param  [in] player プレイヤー番号。
        //! @return 手札の並び。
        HandView& Hand(size_t player) { return m_hands[player]; }

        //! 場の列のカードを返します。
        //! @return 場の列（年代順）。
        const std::vector<Tsukino::ECS::Entity>& Row() const { return m_row; }

        //! 場の列の左端と右端の X 座標を返します（カメラの横移動の範囲に使う）。
        //! @return 左端と右端。
        hlslpp::float2 RowExtent() const;

        //! 手札のカードの、並びの中での姿勢を計算します（持ち上げ・ホバーを含む）。
        //! @param  [in]  player   プレイヤー番号。
        //! @param  [in]  index    手札の番号。
        //! @param  [out] position 位置。
        //! @param  [out] rotation 向き。
        //! @param  [out] scale    拡大率。
        void HandPose(size_t player, size_t index, hlslpp::float3& position, hlslpp::quaternion& rotation, float& scale) const;

        //! 山札の一番上の位置を返します。
        //! @return ワールド座標。
        hlslpp::float3 DeckTop() const;

        //! 場の列でカーソルが重なっているカードを設定します（持ち上げて年号を読みやすくする）。
        //! @param  [in] index 場の何枚目か。-1 で解除。
        void SetRowHovered(int index) { m_rowHovered = index; }

        //! 並べ直しの対象から外すカード（ドラッグ中・公開中など）を設定します。
        //! @param  [in] entity 外すカード。entt::null で解除。
        void SetHeld(Tsukino::ECS::Entity entity) { m_held = entity; }

    private:

        //! 場の列のカードの間隔を返します。枚数が増えるほど詰めます。
        //! @return 間隔。
        float RowSpacing() const;

        //! 場の列の i 枚目の X 座標を返します（隙間を含む）。
        //! @param  [in] index 何枚目か。
        //! @return X 座標。
        float RowCardX(size_t index) const;

        CardFactory&                      m_factory;                    // カードの生成器
        std::vector<Tsukino::ECS::Entity> m_row;                        // 場の列（年代順）
        std::vector<HandView>             m_hands;                      // 各プレイヤーの手札
        std::vector<Tsukino::ECS::Entity> m_discards;                   // 捨て札の山
        std::vector<hlslpp::float4>       m_discardJitter;              // 捨て札のずれ（x, z, 回転, 高さ）
        std::vector<Tsukino::ECS::Entity> m_deckCovers;                 // 山札の厚みを見せる裏向きのカード
        Tsukino::ECS::Entity              m_marker = entt::null;        // 挿入位置の目印
        Tsukino::ECS::Entity              m_held   = entt::null;        // 並べ直しから外すカード
        std::optional<size_t>             m_gap;                        // 隙間を空けているスロット
        float                             m_gapWidth = 0.0f;            // 隙間の今の広さ（なめらかに開閉する）
        float                             m_time     = 0.0f;            // 経過時間（揺れの演出用）
        int                               m_rowHovered = -1;            // 場の列でカーソルが重なっているカード
        std::mt19937                      m_rng;                        // 捨て札のずれ用の乱数
    };

}    // namespace ChronoSort::Board
