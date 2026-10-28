//----------------------------------------------------------------------------
//! @file   BoardView.cpp
//! @brief  卓の上のカードの並べ方の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Board/BoardView.hpp>

#include <ChronoSort/ECS/CardMotion.hpp>
#include <ChronoSort/ECS/Component/CardMotionComponent.hpp>
#include <ChronoSort/ECS/System/CardGlowSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

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
        constexpr float kSeatZ       = 300.0f;
        constexpr float kSeatScale   = 0.72f;
        constexpr float kSeatSpacing = 34.0f;

        // 山札・捨て札。場の列の両端に置くと長くなった列とぶつかるため、列より手前の左右の角に置く
        const hlslpp::float3 kDeckPosition(350.0f, 0.0f, -120.0f);
        const hlslpp::float3 kDiscardPosition(-350.0f, 0.0f, -115.0f);
        constexpr float      kCoverThickness = 1.4f;    // 山札の1枚分の見た目の厚み
        constexpr size_t     kMaxCovers      = 9;

        // 場の列。少ないうちは全部を大きく並べ、増えたら注目点（カーソル・隙間）の周りだけ大きく広げて
        // 遠いカードほど細く詰める。それでも収まらなければ流す
        constexpr float kRowScaleMax   = 1.4f;      // 通常の拡大率（注目点の周りもこの大きさ）
        constexpr float kRowScaleFar   = 0.9f;      // 注目点から遠いカードの拡大率の下限（位置関係が分かれば足りる）
        constexpr float kRowSpacingMax = 74.0f;     // 拡大率 1 のときの、重ならない間隔
        constexpr float kNarrowMin     = 14.0f;     // 遠いカードを詰める間隔の下限（縁が覗く程度）
        constexpr float kFocusRadius   = 2.2f;      // 注目点の周りで広げる範囲（枚数）
        constexpr float kGapWidth      = 60.0f;     // 挿入位置の隙間の広さ（拡大率 1 のとき）
        constexpr float kRowHalfSpan   = 430.0f;   // カードの中心を置いてよい左右の範囲（画面の端で切れない幅）
        constexpr float kFoldDepth     = 26.0f;     // 範囲の外のカードを端に畳んで重ねる奥行き
        constexpr float kFitSharpness  = 6.0f;      // 拡大率・間隔・流す量の追従の速さ

        //----------------------------------------------------------------------------
        //! エンティティごとに決まった -0.5〜0.5 の値を返します（列のカードの僅かな傾きに使う）。
        //! @param  [in] entity エンティティ。
        //! @param  [in] salt   用途ごとに値を変えるための数。
        //! @return -0.5〜0.5 の値。
        //----------------------------------------------------------------------------
        float StableJitter(Tsukino::ECS::Entity entity, std::uint32_t salt) {
            std::uint32_t h = static_cast<std::uint32_t>(entity) * 2654435761u + salt * 40503u;
            h ^= h >> 13;
            h *= 0x5bd1e995u;
            h ^= h >> 15;
            return static_cast<float>(h % 10000u) / 10000.0f - 0.5f;
        }

        //----------------------------------------------------------------------------
        //! 場の列に置いたカードの姿勢を計算します（手前の辺を卓に付けたまま kRowTilt だけ起こす）。
        //! @param  [in]  x        X 座標。
        //! @param  [in]  z        寝かせたときの中心の Z 座標。
        //! @param  [in]  yaw      卓の上でのわずかな向きのばらつき。
        //! @param  [in]  scale    拡大率。
        //! @param  [out] position 位置。
        //! @param  [out] rotation 向き。
        //----------------------------------------------------------------------------
        void RowRestPose(float x, float z, float yaw, float scale, hlslpp::float3& position, hlslpp::quaternion& rotation) {
            const float halfHeight = kCardHeight * 0.5f * scale;
            position = hlslpp::float3(x, kTableSurfaceY + halfHeight * std::sin(kRowTilt), z - halfHeight * (1.0f - std::cos(kRowTilt)));
            rotation = hlslpp::mul(hlslpp::quaternion::rotation_x(-kRowTilt), hlslpp::mul(hlslpp::quaternion::rotation_y(yaw), RevealedRotation()));
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! カードの生成器を受け取ります。
    //----------------------------------------------------------------------------
    BoardView::BoardView(CardFactory& factory)
        : m_factory(factory)
        , m_rng(std::random_device{}()) {
    }

    //----------------------------------------------------------------------------
    //! 卓・山札・挿入位置の目印を作ります。
    //----------------------------------------------------------------------------
    void BoardView::Build(size_t playerCount) {
        m_factory.CreateTable();
        m_marker = m_factory.CreateMarker();

        m_hands.assign(playerCount, HandView{});
        // 奥の席は人数に応じて左右に振り分ける
        const size_t cpuCount = playerCount > 0 ? playerCount - 1 : 0;
        for(size_t i = 0; i < cpuCount; ++i) {
            const float spread       = (cpuCount == 1) ? 0.0f : 320.0f;
            const float t            = (cpuCount == 1) ? 0.0f : (static_cast<float>(i) / static_cast<float>(cpuCount - 1)) * 2.0f - 1.0f;
            m_hands[i + 1].style     = HandStyle::Seat;
            m_hands[i + 1].seat      = hlslpp::float3(t * spread, 0.0f, kSeatZ - std::abs(t) * 30.0f);
        }
    }

    //----------------------------------------------------------------------------
    //! 山札の位置にカードを生成します。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity BoardView::SpawnAtDeck(const Rules::CardData& card) {
        return m_factory.CreateCard(card, DeckTop(), hlslpp::quaternion::identity());
    }

    //----------------------------------------------------------------------------
    //! 山札の一番上の位置を返します。
    //----------------------------------------------------------------------------
    hlslpp::float3 BoardView::DeckTop() const {
        return kDeckPosition + hlslpp::float3(0.0f, kTableSurfaceY + static_cast<float>(m_deckCovers.size()) * kCoverThickness + 1.0f, 0.0f);
    }

    //----------------------------------------------------------------------------
    //! 毎フレームの並べ直しを行います。
    //----------------------------------------------------------------------------
    void BoardView::UpdateLayout(float deltaTime) {
        Tsukino::ECS::Registry& registry = m_factory.Registry();
        m_time += deltaTime;

        // 注目点と拡大の強さ、流す量はなめらかに追う
        const float fitRate = 1.0f - std::exp(-kFitSharpness * deltaTime);

        // 注目点は、カーソル（持っているカード）を優先し、無ければ隙間（CPU の迷い・公開中）にする。
        // カーソルのときは、広げたあとの並びでカーソルの真下に注目点が来る位置を解く（広げるたびにずれていかないように）。
        // どちらも無いときは最後に見ていた場所を広げたままにする（全体を均等に詰めると、どこも読めなくなるため）
        float focusTarget = m_focus;
        if(m_focusX) {
            focusTarget = FocusUnderX(std::clamp(*m_focusX, -kRowHalfSpan, kRowHalfSpan) + m_scroll);
        } else if(m_gap) {
            focusTarget = static_cast<float>(*m_gap);
        }
        // 広げ始める前は、広げる場所へその場で移す（列の端から滑ってこないように）
        if(m_focusStrength < 0.02f) {
            m_focus = focusTarget;
        } else {
            m_focus += (focusTarget - m_focus) * fitRate;
        }
        m_focusStrength += (1.0f - m_focusStrength) * fitRate;

        // 隙間はなめらかに開閉する
        const float gapTarget = m_gap ? kGapWidth * SlotScale(*m_gap) : 0.0f;
        m_gapWidth += (gapTarget - m_gapWidth) * (1.0f - std::exp(-14.0f * deltaTime));

        BuildLayout(m_focus, m_focusStrength, m_layout);
        m_scrollTarget = std::clamp(m_scrollTarget, -ScrollLimit(), ScrollLimit());
        m_scroll      += (m_scrollTarget - m_scroll) * fitRate;

        // 場の列。わずかに向きをばらして、定規で並べたような硬さを消す
        for(size_t i = 0; i < m_row.size(); ++i) {
            const Tsukino::ECS::Entity entity = m_row[i];
            if(entity == m_held) {
                continue;
            }
            const float weight    = m_layout.weight[i];
            const float cardScale = m_layout.farScale + (kRowScaleMax - m_layout.farScale) * weight;
            float       fold      = 0.0f;
            const float x         = FoldX(BoundaryX(m_layout, static_cast<float>(i) + 0.5f), fold);
            // 注目点に近いカードほど手前に出して、遠いカードの上に重ねる
            const float z = kRowZ + StableJitter(entity, 2) * 6.0f - weight * 12.0f;
            if(static_cast<int>(i) == m_rowHovered) {
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
        if(m_marker != entt::null && m_gap) {
            ECS::CardMotion::SetTarget(registry, m_marker, SlotPosition(*m_gap) - hlslpp::float3(0.0f, 0.15f, 0.0f), hlslpp::quaternion::identity());
        }

        // 手札
        for(size_t player = 0; player < m_hands.size(); ++player) {
            const HandView& hand = m_hands[player];
            for(size_t i = 0; i < hand.cards.size(); ++i) {
                if(hand.cards[i] == m_held) {
                    continue;
                }
                hlslpp::float3     position;
                hlslpp::quaternion rotation;
                float              scale = 1.0f;
                HandPose(player, i, position, rotation, scale);
                ECS::CardMotion::SetTarget(registry, hand.cards[i], position, rotation, scale);
            }
        }

        // 捨て札の山（雑に積まれた感じを残す）
        for(size_t i = 0; i < m_discards.size(); ++i) {
            if(m_discards[i] == m_held) {
                continue;
            }
            const hlslpp::float4     jitter   = m_discardJitter[i];
            const hlslpp::quaternion rotation = hlslpp::mul(hlslpp::quaternion::rotation_y(jitter.z), RevealedRotation());
            const hlslpp::float3     position = kDiscardPosition + hlslpp::float3(jitter.x, kTableSurfaceY + static_cast<float>(i) * 0.8f, jitter.y);
            ECS::CardMotion::SetTarget(registry, m_discards[i], position, rotation);
        }
    }

    //----------------------------------------------------------------------------
    //! 手札のカードの、並びの中での姿勢を計算します。
    //----------------------------------------------------------------------------
    void BoardView::HandPose(size_t player, size_t index, hlslpp::float3& position, hlslpp::quaternion& rotation, float& scale) const {
        const HandView& hand   = m_hands[player];
        const float     count  = static_cast<float>(hand.cards.size());
        const float     offset = static_cast<float>(index) - (count - 1.0f) * 0.5f;
        const bool      isHovered = hand.hovered == static_cast<int>(index);
        const bool      isLifted  = hand.lifted == static_cast<int>(index);

        if(hand.style == HandStyle::Seat) {
            position           = hand.seat + hlslpp::float3(offset * kSeatSpacing, kTableSurfaceY + static_cast<float>(index) * 0.7f, 0.0f);
            rotation           = hlslpp::quaternion::rotation_y(offset * 0.06f + StableJitter(hand.cards[index], 3) * 0.08f);
            scale              = kSeatScale;
            if(isLifted) {
                // 思考中：手に取ってカメラへ傾け、迷うように揺らす
                position += hlslpp::float3(0.0f, 55.0f, -40.0f);
                const hlslpp::quaternion sway = hlslpp::quaternion::rotation_z(std::sin(m_time * 4.0f) * 0.06f);
                rotation                      = hlslpp::mul(sway, hlslpp::quaternion::rotation_x(-0.6f));
                scale                         = 0.95f;
            }
            return;
        }

        const float spacing = std::min(kHandSpacing, kHandMaxSpread / std::max(count, 1.0f));
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
            position = hlslpp::float3(x * 0.4f, kHandBaseY + 70.0f + std::sin(m_time * 2.4f) * 3.0f, kHandCenterZ + 70.0f);
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
    //! 注目点と拡大の強さから、場の列の並びを求めます。
    //----------------------------------------------------------------------------
    void BoardView::BuildLayout(float focus, float strength, RowLayout& layout) const {
        const size_t count = m_row.size();
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
        const float narrow = std::clamp((span - m_gapWidth - wide * weightSum) / rest, kNarrowMin, wide);
        layout.farScale    = std::clamp(narrow / kRowSpacingMax, kRowScaleFar, kRowScaleMax);

        layout.total = m_gapWidth;
        for(size_t i = 0; i < count; ++i) {
            layout.width[i] = narrow + (wide - narrow) * layout.weight[i];
            layout.total   += layout.width[i];
        }
    }

    //----------------------------------------------------------------------------
    //! 今の並びを返します。
    //----------------------------------------------------------------------------
    const BoardView::RowLayout& BoardView::Layout() const {
        // 列に入れた直後など、まだ並べ直していないときはその場で求める
        if(m_layout.width.size() != m_row.size()) {
            BuildLayout(m_focus, m_focusStrength, m_layout);
        }
        return m_layout;
    }

    //----------------------------------------------------------------------------
    //! 並びの中の位置 t の、流す前の X 座標を返します。
    //----------------------------------------------------------------------------
    float BoardView::BoundaryX(const RowLayout& layout, float t) const {
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
        if(m_gap) {
            x += m_gapWidth * std::clamp(t - static_cast<float>(*m_gap) + 0.5f, 0.0f, 1.0f);
        }
        return x;
    }

    //----------------------------------------------------------------------------
    //! 流す前の X 座標が、注目点の真下に来るような注目点を求めます。
    //----------------------------------------------------------------------------
    float BoardView::FocusUnderX(float x) const {
        // 注目点を右へ動かすほど、その注目点の位置も右へ動く（単調）ので、二分探索で解ける
        const float count = static_cast<float>(m_row.size());
        float       lo    = 0.0f;
        float       hi    = count;
        RowLayout   layout;
        for(int i = 0; i < 20; ++i) {
            const float mid = (lo + hi) * 0.5f;
            BuildLayout(mid, 1.0f, layout);
            if(BoundaryX(layout, mid) < x) {
                lo = mid;
            } else {
                hi = mid;
            }
        }
        return (lo + hi) * 0.5f;
    }

    //----------------------------------------------------------------------------
    //! 場の列のスロットに置くカードの拡大率を返します。
    //----------------------------------------------------------------------------
    float BoardView::SlotScale(size_t slot) const {
        const RowLayout& layout = Layout();
        const float      d      = (static_cast<float>(slot) - layout.focus) / kFocusRadius;
        const float      weight = layout.strength * std::exp(-d * d);
        return layout.farScale + (kRowScaleMax - layout.farScale) * weight;
    }

    //----------------------------------------------------------------------------
    //! 場の列を流せる量の上限を返します。
    //----------------------------------------------------------------------------
    float BoardView::ScrollLimit() const {
        return std::max(0.0f, Layout().total * 0.5f - kRowHalfSpan);
    }

    //----------------------------------------------------------------------------
    //! 場の列を横に流します。
    //----------------------------------------------------------------------------
    void BoardView::ScrollBy(float amount) {
        m_scrollTarget = std::clamp(m_scrollTarget + amount, -ScrollLimit(), ScrollLimit());
    }

    //----------------------------------------------------------------------------
    //! 指定したスロットが画面の中ほどに来るよう、場の列を流します。
    //----------------------------------------------------------------------------
    void BoardView::ScrollToSlot(size_t slot) {
        const float x = BoundaryX(Layout(), static_cast<float>(slot));
        // 範囲内に見えているなら動かさない（収まっている列が無駄に揺れないように）
        if(std::abs(x - m_scrollTarget) > kRowHalfSpan * 0.6f) {
            m_scrollTarget = std::clamp(x, -ScrollLimit(), ScrollLimit());
        }
    }

    //----------------------------------------------------------------------------
    //! 流す前の X 座標を、流して端で畳んだあとの X 座標に変えます。
    //----------------------------------------------------------------------------
    float BoardView::FoldX(float x, float& fold) const {
        const float scrolled = x - m_scroll;
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
    //! 場の列のスロットの中心を返します。
    //----------------------------------------------------------------------------
    hlslpp::float3 BoardView::SlotPosition(size_t slot) const {
        float       fold = 0.0f;
        const float x    = FoldX(BoundaryX(Layout(), static_cast<float>(slot)), fold);
        return hlslpp::float3(x, kTableSurfaceY, kRowZ);
    }

    //----------------------------------------------------------------------------
    //! 場の列のスロットに置いたカードの姿勢を返します。
    //----------------------------------------------------------------------------
    void BoardView::SlotRestPose(size_t slot, hlslpp::float3& position, hlslpp::quaternion& rotation, float& scale) const {
        const hlslpp::float3 center = SlotPosition(slot);
        scale                       = SlotScale(slot);
        RowRestPose(center.x, kRowZ, 0.0f, scale, position, rotation);
    }

    //----------------------------------------------------------------------------
    //! ワールドの X 座標から、最も近いスロットを返します。
    //----------------------------------------------------------------------------
    size_t BoardView::SlotFromWorldX(float worldX) const {
        // 端に畳んだ部分を指したときは、見えている端のスロットにする（続きは流して見せる）
        const float      x      = std::clamp(worldX, -kRowHalfSpan, kRowHalfSpan) + m_scroll;
        const RowLayout& layout = Layout();
        // 広げたあとの見た目の境目のうち、いちばん近いもの
        size_t best         = 0;
        float  bestDistance = 1e30f;
        for(size_t slot = 0; slot <= m_row.size(); ++slot) {
            const float distance = std::abs(BoundaryX(layout, static_cast<float>(slot)) - x);
            if(distance < bestDistance) {
                bestDistance = distance;
                best         = slot;
            }
        }
        return best;
    }

    //----------------------------------------------------------------------------
    //! レイが当たる手札のカードを探します。
    //----------------------------------------------------------------------------
    int BoardView::PickHandCard(size_t player, const Tsukino::BuiltIn::ECS::CameraUtility::Ray& ray, hlslpp::float2& uv) const {
        Tsukino::ECS::Registry& registry = m_factory.Registry();
        const HandView&         hand     = m_hands[player];
        int                     best     = -1;
        float                   bestDistance = 1e30f;
        for(size_t i = 0; i < hand.cards.size(); ++i) {
            const auto&    transform = registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(hand.cards[i]);
            float          distance  = 0.0f;
            hlslpp::float2 hit;
            if(Tsukino::BuiltIn::ECS::CameraUtility::IntersectRect(ray, transform.worldMatrix, hlslpp::float3(1.0f, 0.0f, 0.0f), hlslpp::float3(0.0f, 0.0f, 1.0f),
                                                                   hlslpp::float2(kCardWidth, kCardHeight), distance, hit) &&
               distance < bestDistance) {
                bestDistance = distance;
                best         = static_cast<int>(i);
                uv           = hit;
            }
        }
        return best;
    }

    //----------------------------------------------------------------------------
    //! レイが当たる場のカードを探します。
    //----------------------------------------------------------------------------
    int BoardView::PickRowCard(const Tsukino::BuiltIn::ECS::CameraUtility::Ray& ray) const {
        Tsukino::ECS::Registry& registry     = m_factory.Registry();
        int                     best         = -1;
        float                   bestDistance = 1e30f;
        for(size_t i = 0; i < m_row.size(); ++i) {
            const auto&    transform = registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(m_row[i]);
            float          distance  = 0.0f;
            hlslpp::float2 hit;
            // 起こしたカードが隣に重なるので、いちばん手前を選ぶ
            if(Tsukino::BuiltIn::ECS::CameraUtility::IntersectRect(ray, transform.worldMatrix, hlslpp::float3(1.0f, 0.0f, 0.0f), hlslpp::float3(0.0f, 0.0f, 1.0f),
                                                                   hlslpp::float2(kCardWidth, kCardHeight), distance, hit) &&
               distance < bestDistance) {
                bestDistance = distance;
                best         = static_cast<int>(i);
            }
        }
        return best;
    }

    //----------------------------------------------------------------------------
    //! 隙間に置く目印の表示を切り替えます。
    //----------------------------------------------------------------------------
    void BoardView::SetMarker(bool visible, bool warning) {
        Tsukino::ECS::Registry& registry = m_factory.Registry();
        auto&                   model    = registry.GetComponent<Tsukino::BuiltIn::ECS::ModelComponent>(m_marker);
        if(visible && !model.visible && m_gap) {
            // 前回の位置から滑ってこないよう、出す瞬間はその場へ移す
            auto& motion       = registry.GetComponent<ECS::CardMotionComponent>(m_marker);
            motion.position    = SlotPosition(*m_gap);
            motion.targetPosition = motion.position;
        }
        model.visible = visible;
        m_factory.SetMarkerWarning(m_marker, warning);
        if(visible) {
            ECS::PulseGlow(registry, m_marker, warning ? hlslpp::float3(1.2f, 0.12f, 0.08f) : hlslpp::float3(0.9f, 0.65f, 0.22f), 1.0f, 6.0f);
        } else {
            ECS::StopGlow(registry, m_marker);
        }
    }

    //----------------------------------------------------------------------------
    //! 公開を終えたカードを場の列に入れます。
    //----------------------------------------------------------------------------
    void BoardView::InsertIntoRow(size_t slot, Tsukino::ECS::Entity entity) {
        slot = std::min(slot, m_row.size());
        m_row.insert(m_row.begin() + static_cast<std::ptrdiff_t>(slot), entity);
        // 隙間に入ったので閉じる（隙間の広さは列の間隔に引き継がれる）
        m_gap      = std::nullopt;
        m_gapWidth = 0.0f;
    }

    //----------------------------------------------------------------------------
    //! カードを捨て札の山に送ります。
    //----------------------------------------------------------------------------
    void BoardView::SendToDiscard(Tsukino::ECS::Entity entity) {
        std::uniform_real_distribution<float> unit(-1.0f, 1.0f);
        m_discards.push_back(entity);
        m_discardJitter.push_back(hlslpp::float4(unit(m_rng) * 14.0f, unit(m_rng) * 14.0f, unit(m_rng) * 0.35f, 0.0f));

        Tsukino::ECS::Registry& registry = m_factory.Registry();
        const size_t            index    = m_discards.size() - 1;
        const hlslpp::float4    jitter   = m_discardJitter[index];
        ECS::CardMotion::SetTarget(registry, entity,
                                   kDiscardPosition + hlslpp::float3(jitter.x, kTableSurfaceY + static_cast<float>(index) * 0.8f, jitter.y),
                                   hlslpp::mul(hlslpp::quaternion::rotation_y(jitter.z), RevealedRotation()));
        ECS::CardMotion::Fly(registry, entity, 0.55f, 90.0f);
    }

    //----------------------------------------------------------------------------
    //! 山札の残り枚数に合わせて、山の厚みを変えます。
    //----------------------------------------------------------------------------
    void BoardView::SetDeckCount(size_t remaining) {
        const size_t wanted = remaining == 0 ? 0 : std::min(kMaxCovers, (remaining + 4) / 5);
        while(m_deckCovers.size() > wanted) {
            m_factory.Destroy(m_deckCovers.back());
            m_deckCovers.pop_back();
        }
        std::uniform_real_distribution<float> unit(-1.0f, 1.0f);
        while(m_deckCovers.size() < wanted) {
            const float          y        = kTableSurfaceY + static_cast<float>(m_deckCovers.size()) * kCoverThickness;
            const hlslpp::float3 position = kDeckPosition + hlslpp::float3(unit(m_rng) * 1.5f, y, unit(m_rng) * 1.5f);
            m_deckCovers.push_back(m_factory.CreateCover(position, hlslpp::quaternion::rotation_y(unit(m_rng) * 0.05f)));
        }
    }

}    // namespace ChronoSort::Board
