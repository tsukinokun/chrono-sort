//----------------------------------------------------------------------------
//! @file   BoardLayout.cpp
//! @brief  卓の上の並べ方の計算の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Board/BoardLayout.hpp>

#include <ChronoSort/Board/CardFactory.hpp>
#include <ChronoSort/ECS/CardMotion.hpp>
#include <ChronoSort/Game/TuningSet.hpp>

#include <algorithm>
#include <cmath>

namespace ChronoSort::Board {

    //----------------------------------------------------------------------------
    //! 卓の上の並べ方の調整値を返します。
    //----------------------------------------------------------------------------
    const BoardTuning& Tuning() {
        return Game::Tuning().Board();
    }

    //----------------------------------------------------------------------------
    //! CPU の席の位置を返します。
    //----------------------------------------------------------------------------
    hlslpp::float3 SeatPosition(size_t seat, size_t cpuCount) {
        const BoardTuning& tuning = Tuning();
        if(cpuCount <= 1) {
            return hlslpp::float3(0.0f, 0.0f, tuning.seatZ);
        }
        // -1（左端）〜 1（右端）
        const float side = (static_cast<float>(seat) / static_cast<float>(cpuCount - 1)) * 2.0f - 1.0f;
        return hlslpp::float3(side * tuning.seatSpread, 0.0f, tuning.seatZ - std::abs(side) * tuning.seatPullBack);
    }

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
        const BoardTuning& t = Tuning();
        layout.width.assign(count, 0.0f);
        layout.weight.assign(count, 0.0f);
        layout.focus    = focus;
        layout.strength = strength;

        // 注目点に近いほど 1 に近い重み（釣り鐘形）
        float weightSum = 0.0f;
        for(size_t i = 0; i < count; ++i) {
            const float d    = (static_cast<float>(i) + 0.5f - focus) / t.focusRadius;
            layout.weight[i] = strength * std::exp(-d * d);
            weightSum       += layout.weight[i];
        }

        // 注目点の周りは重ならない広さにし、残りの幅を遠いカードで分ける。
        // 全部を広く並べても収まる枚数なら、遠いカードも同じ広さになる（少ないうちは今まで通り全部が大きい）
        const float span   = t.rowHalfSpan * 2.0f;
        const float wide   = t.rowSpacingMax * t.rowScaleMax;
        const float rest   = std::max(static_cast<float>(count) - weightSum, 1.0e-3f);
        const float narrow = std::clamp((span - state.gapWidth - wide * weightSum) / rest, t.narrowMin, wide);
        layout.farScale    = std::clamp(narrow / t.rowSpacingMax, t.rowScaleFar, t.rowScaleMax);

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
        const BoardTuning& t = Tuning();
        const float scrolled = x - state.scroll;
        const float excess   = std::abs(scrolled) - t.rowHalfSpan;
        if(excess <= 0.0f) {
            fold = 0.0f;
            return scrolled;
        }
        // 範囲の外は、遠いほど詰まって端に積み重なる
        fold = 1.0f - std::exp(-excess / (t.foldDepth * t.foldFalloff));
        const float sign = scrolled < 0.0f ? -1.0f : 1.0f;
        return sign * (t.rowHalfSpan + t.foldDepth * fold);
    }

    //----------------------------------------------------------------------------
    //! 場の列のスロットに置くカードの拡大率を返します。
    //----------------------------------------------------------------------------
    float SlotScale(BoardState& state, size_t count, size_t slot) {
        const BoardTuning& t = Tuning();
        const RowLayout& layout = CurrentLayout(state, count);
        const float      d      = (static_cast<float>(slot) - layout.focus) / t.focusRadius;
        const float      weight = layout.strength * std::exp(-d * d);
        return layout.farScale + (t.rowScaleMax - layout.farScale) * weight;
    }

    //----------------------------------------------------------------------------
    //! 場の列を流せる量の上限を返します。
    //----------------------------------------------------------------------------
    float ScrollLimit(BoardState& state, size_t count) {
        const BoardTuning& t = Tuning();
        return std::max(0.0f, CurrentLayout(state, count).total * 0.5f - t.rowHalfSpan);
    }

    //----------------------------------------------------------------------------
    //! 場の列のスロットの中心を返します。
    //----------------------------------------------------------------------------
    hlslpp::float3 SlotCenter(BoardState& state, size_t count, size_t slot) {
        const BoardTuning& t = Tuning();
        float       fold = 0.0f;
        const float x    = FoldX(state, BoundaryX(state, CurrentLayout(state, count), static_cast<float>(slot)), fold);
        return hlslpp::float3(x, kTableSurfaceY, t.rowZ);
    }

    //----------------------------------------------------------------------------
    //! 場の列に置いたカードの姿勢を計算します。
    //----------------------------------------------------------------------------
    void RowRestPose(float x, float z, float yaw, float scale, hlslpp::float3& position, hlslpp::quaternion& rotation) {
        const BoardTuning& t = Tuning();
        const float halfHeight = kCardHeight * 0.5f * scale;
        position = hlslpp::float3(x, kTableSurfaceY + halfHeight * std::sin(t.rowTilt), z - halfHeight * (1.0f - std::cos(t.rowTilt)));
        rotation = hlslpp::mul(hlslpp::quaternion::rotation_x(-t.rowTilt), hlslpp::mul(hlslpp::quaternion::rotation_y(yaw), RevealedRotation()));
    }

    //----------------------------------------------------------------------------
    //! 手札のカードの、並びの中での姿勢を計算します。
    //----------------------------------------------------------------------------
    void HandPose(const BoardState& state, size_t player, size_t index, size_t count, Tsukino::ECS::Entity card, hlslpp::float3& position,
                  hlslpp::quaternion& rotation, float& scale) {
        const BoardTuning& t = Tuning();
        const HandState& hand      = state.hands[player];
        const float      total     = static_cast<float>(count);
        const float      offset    = static_cast<float>(index) - (total - 1.0f) * 0.5f;
        const bool       isHovered = hand.hovered == static_cast<int>(index);
        const bool       isLifted  = hand.lifted == static_cast<int>(index);

        if(hand.style == HandStyle::Seat) {
            position = hand.seat + hlslpp::float3(offset * t.seatSpacing, kTableSurfaceY + static_cast<float>(index) * t.seatStackStep, 0.0f);
            rotation = hlslpp::quaternion::rotation_y(offset * t.seatFanYaw + StableJitter(card, 3) * t.seatYawJitter);
            scale    = t.seatScale;
            if(isLifted) {
                // 思考中：手に取ってカメラへ傾け、迷うように揺らす
                position += t.seatLiftedOffset;
                const hlslpp::quaternion sway = hlslpp::quaternion::rotation_z(std::sin(state.time * t.seatSwayFrequency) * t.seatSwayAmplitude);
                rotation                      = hlslpp::mul(sway, hlslpp::quaternion::rotation_x(t.seatLiftedTilt));
                scale                         = t.seatLiftedScale;
            }
            return;
        }

        const float spacing = std::min(t.handSpacing, t.handMaxSpread / std::max(total, 1.0f));
        float       x       = offset * spacing;
        // ホバー中のカードの両隣は、近いほど大きく避ける
        if(hand.hovered >= 0 && !isHovered) {
            const float distance = static_cast<float>(index) - static_cast<float>(hand.hovered);
            x += (distance > 0.0f ? 1.0f : -1.0f) * t.hoverAvoid / std::abs(distance);
        }
        position = hlslpp::float3(x, t.handBaseY + static_cast<float>(index) * t.handStackStep, t.handCenterZ - offset * offset * t.handArcDepth - static_cast<float>(index));

        // 扇の開き（カードの法線まわり）を先に、カメラへ向ける傾きを後に効かせる
        const hlslpp::quaternion fan  = hlslpp::quaternion::rotation_y(offset * t.handFanAngle);
        const hlslpp::quaternion tilt = hlslpp::quaternion::rotation_x(t.handTilt);
        rotation                      = hlslpp::mul(tilt, fan);
        scale                         = 1.0f;

        if(isLifted) {
            // 選択中：手札の上に掲げて、ゆっくり上下させる
            position = hlslpp::float3(x * t.liftedSpread, t.handBaseY + t.liftedHeight + std::sin(state.time * t.liftedBobFrequency) * t.liftedBobAmplitude,
                                      t.handCenterZ + t.liftedForward);
            rotation = hlslpp::quaternion::rotation_x(t.liftedTilt);
            scale    = t.liftedScale;
        } else if(isHovered) {
            // ホバー：手前に浮かせて拡大し、カーソルの位置へ少し傾ける
            position += t.handHoverOffset;
            const hlslpp::quaternion lean = hlslpp::mul(hlslpp::quaternion::rotation_x(hand.hoverTilt.y * t.handHoverLean),
                                                        hlslpp::quaternion::rotation_z(-hand.hoverTilt.x * t.handHoverLean));
            rotation                      = hlslpp::mul(hlslpp::quaternion::rotation_x(t.handTilt + t.handHoverTilt), lean);
            scale                         = t.handHoverScale;
        }
    }

    //----------------------------------------------------------------------------
    //! 捨て札の山の i 枚目の姿勢を計算します。
    //----------------------------------------------------------------------------
    void DiscardPose(size_t index, const hlslpp::float4& jitter, hlslpp::float3& position, hlslpp::quaternion& rotation) {
        const BoardTuning& t = Tuning();
        rotation = hlslpp::mul(hlslpp::quaternion::rotation_y(jitter.z), RevealedRotation());
        position = t.discardPosition + hlslpp::float3(jitter.x, kTableSurfaceY + static_cast<float>(index) * t.discardStackStep, jitter.y);
    }

    //----------------------------------------------------------------------------
    //! 卓の上のカードの目標の姿勢を、置き場所に合わせて書き込みます。
    //----------------------------------------------------------------------------
    void UpdateBoardLayout(Tsukino::ECS::Registry& registry, float deltaTime) {
        const BoardTuning& t = Tuning();
        BoardState& state = registry.GetContext<BoardState>();
        state.time += deltaTime;

        const std::vector<Tsukino::ECS::Entity> row   = CollectZone(registry, ECS::CardZone::Row);
        const size_t                            count = row.size();

        // 注目点と拡大の強さ、流す量はなめらかに追う
        const float fitRate = 1.0f - std::exp(-t.fitSharpness * deltaTime);

        // 注目点は、カーソル（持っているカード）を優先し、無ければ隙間（CPU の迷い・公開中）にする。
        // カーソルのときは、広げたあとの並びでカーソルの真下に注目点が来る位置を解く（広げるたびにずれていかないように）。
        // どちらも無いときは最後に見ていた場所を広げたままにする（全体を均等に詰めると、どこも読めなくなるため）
        float focusTarget = state.focus;
        if(state.focusX) {
            focusTarget = FocusUnderX(state, count, std::clamp(*state.focusX, -t.rowHalfSpan, t.rowHalfSpan) + state.scroll);
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
        const float gapTarget = state.gap ? t.gapWidth * SlotScale(state, count, *state.gap) : 0.0f;
        state.gapWidth += (gapTarget - state.gapWidth) * (1.0f - std::exp(-t.gapSharpness * deltaTime));

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
            const float cardScale = state.layout.farScale + (t.rowScaleMax - state.layout.farScale) * weight;
            float       fold      = 0.0f;
            const float x         = FoldX(state, BoundaryX(state, state.layout, static_cast<float>(i) + 0.5f), fold);
            // 注目点に近いカードほど手前に出して、遠いカードの上に重ねる
            const float z = t.rowZ + StableJitter(entity, 2) * t.rowDepthJitter - weight * t.rowFocusPull;
            if(static_cast<int>(i) == state.rowHovered) {
                // 指したカードは手前に起こして大きくし、年号を読みやすくする
                const hlslpp::quaternion lift = hlslpp::mul(hlslpp::quaternion::rotation_x(-t.rowHoverTilt), RevealedRotation());
                ECS::CardMotion::SetTarget(registry, entity, hlslpp::float3(x, t.rowHoverHeight, z - t.rowHoverForward), lift, cardScale * t.rowHoverScale);
                continue;
            }
            // 端に畳んだカードは少し小さくして奥へ下げ、内側のカードの下に重ねる
            const float        scale = cardScale * (1.0f - t.foldShrink * fold);
            hlslpp::float3     position;
            hlslpp::quaternion rotation;
            RowRestPose(x, z + fold * t.foldSink, StableJitter(entity, 1) * t.rowYawJitter, scale, position, rotation);
            ECS::CardMotion::SetTarget(registry, entity, position, rotation, scale);
        }

        // 挿入位置の目印は隙間に沿って動く
        if(state.marker != entt::null && state.gap) {
            ECS::CardMotion::SetTarget(registry, state.marker, SlotCenter(state, count, *state.gap) - hlslpp::float3(0.0f, t.markerSink, 0.0f),
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
