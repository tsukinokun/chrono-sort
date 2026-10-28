//----------------------------------------------------------------------------
//! @file   BoardLayout.cpp
//! @brief  卓の上の並べ方の計算の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Board/BoardLayout.hpp>

#include <ChronoSort/Board/CardFactory.hpp>
#include <ChronoSort/ECS/CardMotion.hpp>

#include <algorithm>
#include <cmath>

namespace ChronoSort::Board {

    namespace {
        // 人間の手札（手前の扇）
        constexpr float kHandCenterZ   = -230.0f;
        constexpr float kHandBaseY     = 90.0f;
        constexpr float kHandMaxSpread = 330.0f;    // 扇の全体の幅の上限
        constexpr float kHandSpacing   = 56.0f;     // 枚数が少ないときの間隔
        constexpr float kHandTilt      = -0.55f;    // カメラへ向ける傾き（X 軸まわり）
        constexpr float kHandFanAngle  = 0.07f;     // 1枚あたりの扇の開き
        constexpr float kHandArcDepth  = 6.0f;      // 端ほど奥へ下げる量
        constexpr float kHoverAvoid    = 22.0f;     // ホバー中のカードの両隣が避ける量

        // CPU の席（卓の奥）
        constexpr float kSeatScale   = 0.72f;
        constexpr float kSeatSpacing = 34.0f;

        // 場の列。少ないうちは全部を大きく並べ、増えたら注目点（カーソル・隙間）の周りだけ大きく広げて
        // 遠いカードほど細く詰める。それでも収まらなければ流す
        constexpr float kRowScaleMax   = 1.4f;      // 通常の拡大率（注目点の周りもこの大きさ）
        constexpr float kRowScaleFar   = 0.9f;      // 注目点から遠いカードの拡大率の下限（位置関係が分かれば足りる）
        constexpr float kRowSpacingMax = 74.0f;     // 拡大率 1 のときの、重ならない間隔
        constexpr float kNarrowMin     = 14.0f;     // 遠いカードを詰める間隔の下限（縁が覗く程度）
        constexpr float kFocusRadius   = 2.2f;      // 注目点の周りで広げる範囲（枚数）
        constexpr float kGapWidth      = 60.0f;     // 挿入位置の隙間の広さ（拡大率 1 のとき）
        constexpr float kFoldDepth     = 26.0f;     // 範囲の外のカードを端に畳んで重ねる奥行き
        constexpr float kFitSharpness  = 6.0f;      // 拡大率・間隔・流す量の追従の速さ
    }    // namespace

    //----------------------------------------------------------------------------
    //! エンティティごとに決まった -0.5〜0.5 の値を返します。
    //----------------------------------------------------------------------------
    float StableJitter(Tsukino::ECS::Entity entity, std::uint32_t salt) {
        std::uint32_t h = static_cast<std::uint32_t>(entity) * 2654435761u + salt * 40503u;
        h ^= h >> 13;
        h *= 0x5bd1e995u;
        h ^= h >> 15;
        return static_cast<float>(h % 10000u) / 10000.0f - 0.5f;
    }

    //----------------------------------------------------------------------------
    //! 置き場所のカードを、順番どおりに集めます。
    //----------------------------------------------------------------------------
    std::vector<Tsukino::ECS::Entity> CollectZone(Tsukino::ECS::Registry& registry, ECS::CardZone zone, size_t owner) {
        std::vector<std::pair<size_t, Tsukino::ECS::Entity>> found;
        registry.View<ECS::CardPlacementComponent>().each([&](Tsukino::ECS::Entity entity, const ECS::CardPlacementComponent& placement) {
            if(placement.zone == zone && (zone != ECS::CardZone::Hand || placement.owner == owner)) {
                found.emplace_back(placement.index, entity);
            }
        });
        std::sort(found.begin(), found.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

        std::vector<Tsukino::ECS::Entity> cards;
        cards.reserve(found.size());
        for(const auto& [index, entity] : found) {
            cards.push_back(entity);
        }
        return cards;
    }

    //----------------------------------------------------------------------------
    //! 注目点と拡大の強さから、場の列の並びを求めます。
    //----------------------------------------------------------------------------
    void BuildLayout(const BoardState& state, size_t count, float focus, float strength, RowLayout& layout) {
        layout.width.assign(count, 0.0f);
        layout.weight.assign(count, 0.0f);
        layout.focus    = focus;
        layout.strength = strength;

        // 注目点に近いほど 1 に近い重み（釣り鐘形）
        float weightSum = 0.0f;
        for(size_t i = 0; i < count; ++i) {
            const float d    = (static_cast<float>(i) + 0.5f - focus) / kFocusRadius;
            layout.weight[i] = strength * std::exp(-d * d);
            weightSum       += layout.weight[i];
        }

        // 注目点の周りは重ならない広さにし、残りの幅を遠いカードで分ける。
        // 全部を広く並べても収まる枚数なら、遠いカードも同じ広さになる（少ないうちは今まで通り全部が大きい）
        const float span   = kRowHalfSpan * 2.0f;
        const float wide   = kRowSpacingMax * kRowScaleMax;
        const float rest   = std::max(static_cast<float>(count) - weightSum, 1.0e-3f);
        const float narrow = std::clamp((span - state.gapWidth - wide * weightSum) / rest, kNarrowMin, wide);
        layout.farScale    = std::clamp(narrow / kRowSpacingMax, kRowScaleFar, kRowScaleMax);

        layout.total = state.gapWidth;
        for(size_t i = 0; i < count; ++i) {
            layout.width[i] = narrow + (wide - narrow) * layout.weight[i];
            layout.total   += layout.width[i];
        }
    }

    //----------------------------------------------------------------------------
    //! 今の並びを返します。
    //----------------------------------------------------------------------------
    const RowLayout& CurrentLayout(BoardState& state, size_t count) {
        // 列に入れた直後など、まだ並べ直していないときはその場で求める
        if(state.layout.width.size() != count) {
            BuildLayout(state, count, state.focus, state.focusStrength, state.layout);
        }
        return state.layout;
    }

    //----------------------------------------------------------------------------
    //! 並びの中の位置 t の、流す前の X 座標を返します。
    //----------------------------------------------------------------------------
    float BoundaryX(const BoardState& state, const RowLayout& layout, float t) {
        const size_t count = layout.width.size();
        t                  = std::clamp(t, 0.0f, static_cast<float>(count));
        float        x     = -layout.total * 0.5f;
        const size_t whole = static_cast<size_t>(t);
        for(size_t i = 0; i < whole && i < count; ++i) {
            x += layout.width[i];
        }
        if(whole < count) {
            x += layout.width[whole] * (t - static_cast<float>(whole));
        }
        // 隙間は、そのスロットの境目では半分、越えたら全部を足す
        if(state.gap) {
            x += state.gapWidth * std::clamp(t - static_cast<float>(*state.gap) + 0.5f, 0.0f, 1.0f);
        }
        return x;
    }

    //----------------------------------------------------------------------------
    //! 流す前の X 座標が、注目点の真下に来るような注目点を求めます。
    //----------------------------------------------------------------------------
    float FocusUnderX(const BoardState& state, size_t count, float x) {
        // 注目点を右へ動かすほど、その注目点の位置も右へ動く（単調）ので、二分探索で解ける
        float     lo = 0.0f;
        float     hi = static_cast<float>(count);
        RowLayout layout;
        for(int i = 0; i < 20; ++i) {
            const float mid = (lo + hi) * 0.5f;
            BuildLayout(state, count, mid, 1.0f, layout);
            if(BoundaryX(state, layout, mid) < x) {
                lo = mid;
            } else {
                hi = mid;
            }
        }
        return (lo + hi) * 0.5f;
    }

    //----------------------------------------------------------------------------
    //! 流す前の X 座標を、流して端で畳んだあとの X 座標に変えます。
    //----------------------------------------------------------------------------
    float FoldX(const BoardState& state, float x, float& fold) {
        const float scrolled = x - state.scroll;
        const float excess   = std::abs(scrolled) - kRowHalfSpan;
        if(excess <= 0.0f) {
            fold = 0.0f;
            return scrolled;
        }
        // 範囲の外は、遠いほど詰まって端に積み重なる
        fold = 1.0f - std::exp(-excess / (kFoldDepth * 4.0f));
        const float sign = scrolled < 0.0f ? -1.0f : 1.0f;
        return sign * (kRowHalfSpan + kFoldDepth * fold);
    }

    //----------------------------------------------------------------------------
    //! 場の列のスロットに置くカードの拡大率を返します。
    //----------------------------------------------------------------------------
    float SlotScale(BoardState& state, size_t count, size_t slot) {
        const RowLayout& layout = CurrentLayout(state, count);
        const float      d      = (static_cast<float>(slot) - layout.focus) / kFocusRadius;
        const float      weight = layout.strength * std::exp(-d * d);
        return layout.farScale + (kRowScaleMax - layout.farScale) * weight;
    }

    //----------------------------------------------------------------------------
    //! 場の列を流せる量の上限を返します。
    //----------------------------------------------------------------------------
    float ScrollLimit(BoardState& state, size_t count) {
        return std::max(0.0f, CurrentLayout(state, count).total * 0.5f - kRowHalfSpan);
    }

    //----------------------------------------------------------------------------
    //! 場の列のスロットの中心を返します。
    //----------------------------------------------------------------------------
    hlslpp::float3 SlotCenter(BoardState& state, size_t count, size_t slot) {
        float       fold = 0.0f;
        const float x    = FoldX(state, BoundaryX(state, CurrentLayout(state, count), static_cast<float>(slot)), fold);
        return hlslpp::float3(x, kTableSurfaceY, kRowZ);
    }

    //----------------------------------------------------------------------------
    //! 場の列に置いたカードの姿勢を計算します。
    //----------------------------------------------------------------------------
    void RowRestPose(float x, float z, float yaw, float scale, hlslpp::float3& position, hlslpp::quaternion& rotation) {
        const float halfHeight = kCardHeight * 0.5f * scale;
        position = hlslpp::float3(x, kTableSurfaceY + halfHeight * std::sin(kRowTilt), z - halfHeight * (1.0f - std::cos(kRowTilt)));
        rotation = hlslpp::mul(hlslpp::quaternion::rotation_x(-kRowTilt), hlslpp::mul(hlslpp::quaternion::rotation_y(yaw), RevealedRotation()));
    }

    //----------------------------------------------------------------------------
    //! 手札のカードの、並びの中での姿勢を計算します。
    //----------------------------------------------------------------------------
    void HandPose(const BoardState& state, size_t player, size_t index, size_t count, Tsukino::ECS::Entity card, hlslpp::float3& position,
                  hlslpp::quaternion& rotation, float& scale) {
        const HandState& hand      = state.hands[player];
        const float      total     = static_cast<float>(count);
        const float      offset    = static_cast<float>(index) - (total - 1.0f) * 0.5f;
        const bool       isHovered = hand.hovered == static_cast<int>(index);
        const bool       isLifted  = hand.lifted == static_cast<int>(index);

        if(hand.style == HandStyle::Seat) {
            position = hand.seat + hlslpp::float3(offset * kSeatSpacing, kTableSurfaceY + static_cast<float>(index) * 0.7f, 0.0f);
            rotation = hlslpp::quaternion::rotation_y(offset * 0.06f + StableJitter(card, 3) * 0.08f);
            scale    = kSeatScale;
            if(isLifted) {
                // 思考中：手に取ってカメラへ傾け、迷うように揺らす
                position += hlslpp::float3(0.0f, 55.0f, -40.0f);
                const hlslpp::quaternion sway = hlslpp::quaternion::rotation_z(std::sin(state.time * 4.0f) * 0.06f);
                rotation                      = hlslpp::mul(sway, hlslpp::quaternion::rotation_x(-0.6f));
                scale                         = 0.95f;
            }
            return;
        }

        const float spacing = std::min(kHandSpacing, kHandMaxSpread / std::max(total, 1.0f));
        float       x       = offset * spacing;
        // ホバー中のカードの両隣は、近いほど大きく避ける
        if(hand.hovered >= 0 && !isHovered) {
            const float distance = static_cast<float>(index) - static_cast<float>(hand.hovered);
            x += (distance > 0.0f ? 1.0f : -1.0f) * kHoverAvoid / std::abs(distance);
        }
        position = hlslpp::float3(x, kHandBaseY + static_cast<float>(index) * 1.5f, kHandCenterZ - offset * offset * kHandArcDepth - static_cast<float>(index));

        // 扇の開き（カードの法線まわり）を先に、カメラへ向ける傾きを後に効かせる
        const hlslpp::quaternion fan  = hlslpp::quaternion::rotation_y(offset * kHandFanAngle);
        const hlslpp::quaternion tilt = hlslpp::quaternion::rotation_x(kHandTilt);
        rotation                      = hlslpp::mul(tilt, fan);
        scale                         = 1.0f;

        if(isLifted) {
            // 選択中：手札の上に掲げて、ゆっくり上下させる
            position = hlslpp::float3(x * 0.4f, kHandBaseY + 70.0f + std::sin(state.time * 2.4f) * 3.0f, kHandCenterZ + 70.0f);
            rotation = hlslpp::quaternion::rotation_x(-0.75f);
            scale    = 1.15f;
        } else if(isHovered) {
            // ホバー：手前に浮かせて拡大し、カーソルの位置へ少し傾ける
            position += hlslpp::float3(0.0f, 30.0f, -28.0f);
            const hlslpp::quaternion lean = hlslpp::mul(hlslpp::quaternion::rotation_x(hand.hoverTilt.y * 0.35f), hlslpp::quaternion::rotation_z(-hand.hoverTilt.x * 0.35f));
            rotation                      = hlslpp::mul(hlslpp::quaternion::rotation_x(kHandTilt - 0.2f), lean);
            scale                         = 1.22f;
        }
    }

    //----------------------------------------------------------------------------
    //! 捨て札の山の i 枚目の姿勢を計算します。
    //----------------------------------------------------------------------------
    void DiscardPose(size_t index, const hlslpp::float4& jitter, hlslpp::float3& position, hlslpp::quaternion& rotation) {
        rotation = hlslpp::mul(hlslpp::quaternion::rotation_y(jitter.z), RevealedRotation());
        position = kDiscardPosition + hlslpp::float3(jitter.x, kTableSurfaceY + static_cast<float>(index) * 0.8f, jitter.y);
    }

    //----------------------------------------------------------------------------
    //! 卓の上のカードの目標の姿勢を、置き場所に合わせて書き込みます。
    //----------------------------------------------------------------------------
    void UpdateBoardLayout(Tsukino::ECS::Registry& registry, float deltaTime) {
        BoardState& state = registry.GetContext<BoardState>();
        state.time += deltaTime;

        const std::vector<Tsukino::ECS::Entity> row   = CollectZone(registry, ECS::CardZone::Row);
        const size_t                            count = row.size();

        // 注目点と拡大の強さ、流す量はなめらかに追う
        const float fitRate = 1.0f - std::exp(-kFitSharpness * deltaTime);

        // 注目点は、カーソル（持っているカード）を優先し、無ければ隙間（CPU の迷い・公開中）にする。
        // カーソルのときは、広げたあとの並びでカーソルの真下に注目点が来る位置を解く（広げるたびにずれていかないように）。
        // どちらも無いときは最後に見ていた場所を広げたままにする（全体を均等に詰めると、どこも読めなくなるため）
        float focusTarget = state.focus;
        if(state.focusX) {
            focusTarget = FocusUnderX(state, count, std::clamp(*state.focusX, -kRowHalfSpan, kRowHalfSpan) + state.scroll);
        } else if(state.gap) {
            focusTarget = static_cast<float>(*state.gap);
        }
        // 広げ始める前は、広げる場所へその場で移す（列の端から滑ってこないように）
        if(state.focusStrength < 0.02f) {
            state.focus = focusTarget;
        } else {
            state.focus += (focusTarget - state.focus) * fitRate;
        }
        state.focusStrength += (1.0f - state.focusStrength) * fitRate;

        // 隙間はなめらかに開閉する
        const float gapTarget = state.gap ? kGapWidth * SlotScale(state, count, *state.gap) : 0.0f;
        state.gapWidth += (gapTarget - state.gapWidth) * (1.0f - std::exp(-14.0f * deltaTime));

        BuildLayout(state, count, state.focus, state.focusStrength, state.layout);
        state.scrollTarget = std::clamp(state.scrollTarget, -ScrollLimit(state, count), ScrollLimit(state, count));
        state.scroll      += (state.scrollTarget - state.scroll) * fitRate;

        // 場の列。わずかに向きをばらして、定規で並べたような硬さを消す
        for(size_t i = 0; i < count; ++i) {
            const Tsukino::ECS::Entity entity = row[i];
            if(entity == state.held) {
                continue;
            }
            const float weight    = state.layout.weight[i];
            const float cardScale = state.layout.farScale + (kRowScaleMax - state.layout.farScale) * weight;
            float       fold      = 0.0f;
            const float x         = FoldX(state, BoundaryX(state, state.layout, static_cast<float>(i) + 0.5f), fold);
            // 注目点に近いカードほど手前に出して、遠いカードの上に重ねる
            const float z = kRowZ + StableJitter(entity, 2) * 6.0f - weight * 12.0f;
            if(static_cast<int>(i) == state.rowHovered) {
                // 指したカードは手前に起こして大きくし、年号を読みやすくする
                const hlslpp::quaternion lift = hlslpp::mul(hlslpp::quaternion::rotation_x(-0.55f), RevealedRotation());
                ECS::CardMotion::SetTarget(registry, entity, hlslpp::float3(x, 60.0f, z - 60.0f), lift, cardScale * 1.25f);
                continue;
            }
            // 端に畳んだカードは少し小さくして奥へ下げ、内側のカードの下に重ねる
            const float        scale = cardScale * (1.0f - 0.15f * fold);
            hlslpp::float3     position;
            hlslpp::quaternion rotation;
            RowRestPose(x, z + fold * 14.0f, StableJitter(entity, 1) * 0.06f, scale, position, rotation);
            ECS::CardMotion::SetTarget(registry, entity, position, rotation, scale);
        }

        // 挿入位置の目印は隙間に沿って動く
        if(state.marker != entt::null && state.gap) {
            ECS::CardMotion::SetTarget(registry, state.marker, SlotCenter(state, count, *state.gap) - hlslpp::float3(0.0f, 0.15f, 0.0f),
                                       hlslpp::quaternion::identity());
        }

        // 手札
        for(size_t player = 0; player < state.hands.size(); ++player) {
            const std::vector<Tsukino::ECS::Entity> hand = CollectZone(registry, ECS::CardZone::Hand, player);
            for(size_t i = 0; i < hand.size(); ++i) {
                if(hand[i] == state.held) {
                    continue;
                }
                hlslpp::float3     position;
                hlslpp::quaternion rotation;
                float              scale = 1.0f;
                HandPose(state, player, i, hand.size(), hand[i], position, rotation, scale);
                ECS::CardMotion::SetTarget(registry, hand[i], position, rotation, scale);
            }
        }

        // 捨て札の山
        const std::vector<Tsukino::ECS::Entity> discards = CollectZone(registry, ECS::CardZone::Discard);
        for(size_t i = 0; i < discards.size(); ++i) {
            if(discards[i] == state.held) {
                continue;
            }
            hlslpp::float3     position;
            hlslpp::quaternion rotation;
            DiscardPose(i, registry.GetComponent<ECS::CardPlacementComponent>(discards[i]).jitter, position, rotation);
            ECS::CardMotion::SetTarget(registry, discards[i], position, rotation);
        }
    }

}    // namespace ChronoSort::Board
