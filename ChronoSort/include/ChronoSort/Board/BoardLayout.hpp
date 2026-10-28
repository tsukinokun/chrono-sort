//----------------------------------------------------------------------------
//! @file   BoardLayout.hpp
//! @brief  卓の上の並べ方の計算（場の列・手札・捨て札の目標の姿勢）
//! @detail 状態は持たない関数の集まりです。卓全体の状態は BoardState（レジストリのコンテキスト）、
//!         カードの置き場所は CardPlacementComponent から読みます。
//!         毎フレームの並べ直しは BoardLayoutSystem が UpdateBoardLayout を呼んで行い、
//!         入力や演出が使う問い合わせ（スロットの位置など）は BoardView がここを呼びます。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Board/BoardState.hpp>
#include <ChronoSort/Board/BoardTuning.hpp>
#include <ChronoSort/ECS/Component/CardPlacementComponent.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <hlsl++.h>

#include <cstdint>
#include <vector>

namespace ChronoSort::Board {

    // 卓に寝かせたカードの高さ（卓のモデルの板の厚みの半分 + 僅か）。モデルに結びついた値なので調整値にしない
    constexpr float kTableSurfaceY = 0.31f;

    //! 卓の上の並べ方の調整値を返します（Board.json。Game::Tuning().Board() と同じ）。
    //! @return 調整値。
    const BoardTuning& Tuning();

    //! CPU の席の位置を返します（人数に応じて左右に振り分け、端ほど手前へ寄せる）。
    //! @param  [in] seat     席の番号（0 から）。
    //! @param  [in] cpuCount CPU の人数。
    //! @return 席の中心（卓の上）。
    hlslpp::float3 SeatPosition(size_t seat, size_t cpuCount);

    //! エンティティごとに決まった -0.5〜0.5 の値を返します（カードの僅かな傾きに使う）。
    //! @param  [in] entity エンティティ。
    //! @param  [in] salt   用途ごとに値を変えるための数。
    //! @return -0.5〜0.5 の値。
    float StableJitter(Tsukino::ECS::Entity entity, std::uint32_t salt);

    //! 置き場所のカードを、順番どおりに集めます。
    //! @param  [in] registry レジストリ。
    //! @param  [in] zone     置き場所。
    //! @param  [in] owner    手札の持ち主（zone が Hand のとき）。
    //! @return カード（順番どおり）。
    std::vector<Tsukino::ECS::Entity> CollectZone(Tsukino::ECS::Registry& registry, ECS::CardZone zone, size_t owner = 0);

    //! 注目点と拡大の強さから、場の列の並びを求めます。
    //! @param  [in]  state    卓の状態（隙間の広さを使う）。
    //! @param  [in]  count    場の列の枚数。
    //! @param  [in]  focus    注目点。
    //! @param  [in]  strength 拡大の強さ。
    //! @param  [out] layout   並び。
    void BuildLayout(const BoardState& state, size_t count, float focus, float strength, RowLayout& layout);

    //! 今の並びを返します（枚数が変わってまだ並べ直していなければ、その場で求め直す）。
    //! @param  [in,out] state 卓の状態。
    //! @param  [in]     count 場の列の枚数。
    //! @return 並び。
    const RowLayout& CurrentLayout(BoardState& state, size_t count);

    //! 並びの中の位置 t（スロット k の境目が k。小数で途中も表す）の、流す前の X 座標を返します。
    //! @param  [in] state  卓の状態（隙間）。
    //! @param  [in] layout 並び。
    //! @param  [in] t      位置。
    //! @return X 座標。
    float BoundaryX(const BoardState& state, const RowLayout& layout, float t);

    //! 流す前の X 座標が、注目点の真下に来るような注目点を求めます。
    //! @param  [in] state 卓の状態。
    //! @param  [in] count 場の列の枚数。
    //! @param  [in] x     流す前の X 座標。
    //! @return 注目点。
    float FocusUnderX(const BoardState& state, size_t count, float x);

    //! 流す前の X 座標を、流して端で畳んだあとの X 座標に変えます。
    //! @param  [in]  state 卓の状態（流す量）。
    //! @param  [in]  x     流す前の X 座標。
    //! @param  [out] fold  畳まれた深さ（0：畳まれていない 〜 1：いちばん奥）。
    //! @return 画面上の X 座標。
    float FoldX(const BoardState& state, float x, float& fold);

    //! 場の列のスロットに置くカードの拡大率を返します（注目点に近いほど大きい）。
    //! @param  [in,out] state 卓の状態。
    //! @param  [in]     count 場の列の枚数。
    //! @param  [in]     slot  スロット番号。
    //! @return 拡大率。
    float SlotScale(BoardState& state, size_t count, size_t slot);

    //! 場の列を流せる量の上限を返します（列が収まっていれば 0）。
    //! @param  [in,out] state 卓の状態。
    //! @param  [in]     count 場の列の枚数。
    //! @return 上限。
    float ScrollLimit(BoardState& state, size_t count);

    //! 場の列のスロットの中心（卓の上）を返します。今空けている隙間を考慮します。
    //! @param  [in,out] state 卓の状態。
    //! @param  [in]     count 場の列の枚数。
    //! @param  [in]     slot  スロット番号。
    //! @return ワールド座標。
    hlslpp::float3 SlotCenter(BoardState& state, size_t count, size_t slot);

    //! 場の列に置いたカードの姿勢を計算します（手前の辺を卓に付けたまま BoardTuning::rowTilt だけ起こす）。
    //! @param  [in]  x        X 座標。
    //! @param  [in]  z        寝かせたときの中心の Z 座標。
    //! @param  [in]  yaw      卓の上でのわずかな向きのばらつき。
    //! @param  [in]  scale    拡大率。
    //! @param  [out] position 位置。
    //! @param  [out] rotation 向き。
    void RowRestPose(float x, float z, float yaw, float scale, hlslpp::float3& position, hlslpp::quaternion& rotation);

    //! 手札のカードの、並びの中での姿勢を計算します（持ち上げ・ホバーを含む）。
    //! @param  [in]  state    卓の状態。
    //! @param  [in]  player   プレイヤー番号。
    //! @param  [in]  index    手札の番号。
    //! @param  [in]  count    手札の枚数。
    //! @param  [in]  card     カード（席の並びのばらつきに使う）。
    //! @param  [out] position 位置。
    //! @param  [out] rotation 向き。
    //! @param  [out] scale    拡大率。
    void HandPose(const BoardState& state, size_t player, size_t index, size_t count, Tsukino::ECS::Entity card, hlslpp::float3& position,
                  hlslpp::quaternion& rotation, float& scale);

    //! 捨て札の山の i 枚目の姿勢を計算します（雑に積まれた感じを残す）。
    //! @param  [in]  index    捨て札の何枚目か。
    //! @param  [in]  jitter   その札のずれ。
    //! @param  [out] position 位置。
    //! @param  [out] rotation 向き。
    void DiscardPose(size_t index, const hlslpp::float4& jitter, hlslpp::float3& position, hlslpp::quaternion& rotation);

    //! 卓の上のカードの目標の姿勢を、置き場所に合わせて書き込みます（BoardLayoutSystem の本体）。
    //! @param  [in,out] registry  レジストリ（BoardState を持っていること）。
    //! @param  [in]     deltaTime 経過時間（秒）。0 なら注目点などの追従は進めず、今の並びのまま書く。
    void UpdateBoardLayout(Tsukino::ECS::Registry& registry, float deltaTime);

}    // namespace ChronoSort::Board
