//----------------------------------------------------------------------------
//! @file   BoardView.cpp
//! @brief  卓の上のカードの置き場所を変える窓口の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Board/BoardView.hpp>

#include <ChronoSort/ECS/CardMotion.hpp>
#include <ChronoSort/ECS/Component/CardMotionComponent.hpp>
#include <ChronoSort/ECS/Component/CardPlacementComponent.hpp>
#include <ChronoSort/ECS/System/CardGlowSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <algorithm>
#include <cmath>

namespace ChronoSort::Board {

    namespace {
        //----------------------------------------------------------------------------
        //! レイが当たるカードのうち、いちばん手前のものを探します。
        //! @param  [in]  registry レジストリ。
        //! @param  [in]  cards    候補のカード。
        //! @param  [in]  ray      マウスのレイ。
        //! @param  [out] uv       当たった位置のカード上の座標（0〜1）。
        //! @return 候補の何番目か。当たらなければ -1。
        //----------------------------------------------------------------------------
        int PickNearest(Tsukino::ECS::Registry& registry, const std::vector<Tsukino::ECS::Entity>& cards, const Tsukino::BuiltIn::ECS::CameraUtility::Ray& ray,
                        hlslpp::float2& uv) {
            int   best         = -1;
            float bestDistance = 1e30f;
            for(size_t i = 0; i < cards.size(); ++i) {
                const auto&    transform = registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(cards[i]);
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
        //! カードの置き場所を書き換えます（無ければ付けます）。
        //! @param  [in] registry レジストリ。
        //! @param  [in] entity   カード。
        //! @param  [in] zone     置き場所。
        //! @param  [in] owner    手札の持ち主。
        //! @param  [in] index    置き場所の中での順番。
        //! @return 置き場所。
        //----------------------------------------------------------------------------
        ECS::CardPlacementComponent& Place(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, ECS::CardZone zone, size_t owner, size_t index) {
            auto* placement = registry.try_get<ECS::CardPlacementComponent>(entity);
            if(placement == nullptr) {
                placement = &registry.AddComponent<ECS::CardPlacementComponent>(entity);
            }
            placement->zone  = zone;
            placement->owner = owner;
            placement->index = index;
            return *placement;
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
    //! 卓の状態を返します。
    //----------------------------------------------------------------------------
    BoardState& BoardView::State() {
        return m_factory.Registry().GetContext<BoardState>();
    }

    //----------------------------------------------------------------------------
    //! 場の列の枚数を返します。
    //----------------------------------------------------------------------------
    size_t BoardView::RowCount() {
        return CollectZone(m_factory.Registry(), ECS::CardZone::Row).size();
    }

    //----------------------------------------------------------------------------
    //! 卓・挿入位置の目印を作り、卓の状態をレジストリに置きます。
    //----------------------------------------------------------------------------
    void BoardView::Build(size_t playerCount) {
        BoardState& state = m_factory.Registry().SetContext<BoardState>();
        m_factory.CreateTable();
        state.marker = m_factory.CreateMarker();

        state.hands.assign(playerCount, HandState{});
        // 奥の席は人数に応じて左右に振り分ける
        const size_t cpuCount = playerCount > 0 ? playerCount - 1 : 0;
        for(size_t i = 0; i < cpuCount; ++i) {
            const float spread       = (cpuCount == 1) ? 0.0f : 320.0f;
            const float t            = (cpuCount == 1) ? 0.0f : (static_cast<float>(i) / static_cast<float>(cpuCount - 1)) * 2.0f - 1.0f;
            state.hands[i + 1].style = HandStyle::Seat;
            state.hands[i + 1].seat  = hlslpp::float3(t * spread, 0.0f, kSeatZ - std::abs(t) * 30.0f);
        }
    }

    //----------------------------------------------------------------------------
    //! 山札の位置にカードを生成します。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity BoardView::SpawnAtDeck(const Rules::CardData& card) {
        return m_factory.CreateCard(card, DeckTop(), hlslpp::quaternion::identity());
    }

    //----------------------------------------------------------------------------
    //! 今の置き場所どおりの目標を、すぐに書き込みます。
    //----------------------------------------------------------------------------
    void BoardView::ApplyLayoutNow() {
        UpdateBoardLayout(m_factory.Registry(), 0.0f);
    }

    //----------------------------------------------------------------------------
    //! 山札の一番上の位置を返します。
    //----------------------------------------------------------------------------
    hlslpp::float3 BoardView::DeckTop() {
        return kDeckPosition + hlslpp::float3(0.0f, kTableSurfaceY + static_cast<float>(State().deckCovers.size()) * kCoverThickness + 1.0f, 0.0f);
    }

    //----------------------------------------------------------------------------
    //! 場の列のスロットの中心を返します。
    //----------------------------------------------------------------------------
    hlslpp::float3 BoardView::SlotPosition(size_t slot) {
        return SlotCenter(State(), RowCount(), slot);
    }

    //----------------------------------------------------------------------------
    //! 場の列のスロットに置いたカードの姿勢を返します。
    //----------------------------------------------------------------------------
    void BoardView::SlotRestPose(size_t slot, hlslpp::float3& position, hlslpp::quaternion& rotation, float& scale) {
        const hlslpp::float3 center = SlotPosition(slot);
        scale                       = SlotScale(slot);
        RowRestPose(center.x, kRowZ, 0.0f, scale, position, rotation);
    }

    //----------------------------------------------------------------------------
    //! 場の列のスロットに置くカードの拡大率を返します。
    //----------------------------------------------------------------------------
    float BoardView::SlotScale(size_t slot) {
        return Board::SlotScale(State(), RowCount(), slot);
    }

    //----------------------------------------------------------------------------
    //! 場の列を横に流します。
    //----------------------------------------------------------------------------
    void BoardView::ScrollBy(float amount) {
        BoardState& state = State();
        const float limit = ScrollLimit(state, RowCount());
        state.scrollTarget = std::clamp(state.scrollTarget + amount, -limit, limit);
    }

    //----------------------------------------------------------------------------
    //! 指定したスロットが画面の中ほどに来るよう、場の列を流します。
    //----------------------------------------------------------------------------
    void BoardView::ScrollToSlot(size_t slot) {
        BoardState&  state = State();
        const size_t count = RowCount();
        const float  x     = BoundaryX(state, CurrentLayout(state, count), static_cast<float>(slot));
        // 範囲内に見えているなら動かさない（収まっている列が無駄に揺れないように）
        if(std::abs(x - state.scrollTarget) > kRowHalfSpan * 0.6f) {
            const float limit  = ScrollLimit(state, count);
            state.scrollTarget = std::clamp(x, -limit, limit);
        }
    }

    //----------------------------------------------------------------------------
    //! ワールドの X 座標から、最も近いスロットを返します。
    //----------------------------------------------------------------------------
    size_t BoardView::SlotFromWorldX(float worldX) {
        BoardState&      state  = State();
        const size_t     count  = RowCount();
        // 端に畳んだ部分を指したときは、見えている端のスロットにする（続きは流して見せる）
        const float      x      = std::clamp(worldX, -kRowHalfSpan, kRowHalfSpan) + state.scroll;
        const RowLayout& layout = CurrentLayout(state, count);
        // 広げたあとの見た目の境目のうち、いちばん近いもの
        size_t best         = 0;
        float  bestDistance = 1e30f;
        for(size_t slot = 0; slot <= count; ++slot) {
            const float distance = std::abs(BoundaryX(state, layout, static_cast<float>(slot)) - x);
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
    int BoardView::PickHandCard(size_t player, const Tsukino::BuiltIn::ECS::CameraUtility::Ray& ray, hlslpp::float2& uv) {
        return PickNearest(m_factory.Registry(), CollectZone(m_factory.Registry(), ECS::CardZone::Hand, player), ray, uv);
    }

    //----------------------------------------------------------------------------
    //! レイが当たる場のカードを探します。
    //----------------------------------------------------------------------------
    int BoardView::PickRowCard(const Tsukino::BuiltIn::ECS::CameraUtility::Ray& ray) {
        // 起こしたカードが隣に重なるので、いちばん手前を選ぶ
        hlslpp::float2 uv;
        return PickNearest(m_factory.Registry(), CollectZone(m_factory.Registry(), ECS::CardZone::Row), ray, uv);
    }

    //----------------------------------------------------------------------------
    //! 隙間に置く目印の表示を切り替えます。
    //----------------------------------------------------------------------------
    void BoardView::SetMarker(bool visible, bool warning) {
        Tsukino::ECS::Registry& registry = m_factory.Registry();
        BoardState&             state    = State();
        auto&                   model    = registry.GetComponent<Tsukino::BuiltIn::ECS::ModelComponent>(state.marker);
        if(visible && !model.visible && state.gap) {
            // 前回の位置から滑ってこないよう、出す瞬間はその場へ移す
            auto& motion          = registry.GetComponent<ECS::CardMotionComponent>(state.marker);
            motion.position       = SlotPosition(*state.gap);
            motion.targetPosition = motion.position;
        }
        model.visible = visible;
        m_factory.SetMarkerWarning(state.marker, warning);
        if(visible) {
            ECS::PulseGlow(registry, state.marker, warning ? hlslpp::float3(1.2f, 0.12f, 0.08f) : hlslpp::float3(0.9f, 0.65f, 0.22f), 1.0f, 6.0f);
        } else {
            ECS::StopGlow(registry, state.marker);
        }
    }

    //----------------------------------------------------------------------------
    //! 公開を終えたカードを場の列に入れます。
    //----------------------------------------------------------------------------
    void BoardView::InsertIntoRow(size_t slot, Tsukino::ECS::Entity entity) {
        Tsukino::ECS::Registry& registry = m_factory.Registry();
        const std::vector<Tsukino::ECS::Entity> row = CollectZone(registry, ECS::CardZone::Row);
        slot = std::min(slot, row.size());
        // 入れる位置より後ろのカードを1つずつ後ろへずらす
        for(size_t i = slot; i < row.size(); ++i) {
            registry.GetComponent<ECS::CardPlacementComponent>(row[i]).index = i + 1;
        }
        Place(registry, entity, ECS::CardZone::Row, 0, slot);

        // 隙間に入ったので閉じる（隙間の広さは列の間隔に引き継がれる）
        BoardState& state = State();
        state.gap         = std::nullopt;
        state.gapWidth    = 0.0f;
    }

    //----------------------------------------------------------------------------
    //! カードを手札の最後に加えます。
    //----------------------------------------------------------------------------
    void BoardView::AddToHand(size_t player, Tsukino::ECS::Entity entity) {
        Place(m_factory.Registry(), entity, ECS::CardZone::Hand, player, HandSize(player));
    }

    //----------------------------------------------------------------------------
    //! 手札からカードを取り出します。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity BoardView::TakeFromHand(size_t player, size_t index) {
        Tsukino::ECS::Registry& registry = m_factory.Registry();
        const std::vector<Tsukino::ECS::Entity> hand = CollectZone(registry, ECS::CardZone::Hand, player);
        const Tsukino::ECS::Entity entity = hand[index];
        Place(registry, entity, ECS::CardZone::None, 0, 0);
        for(size_t i = index + 1; i < hand.size(); ++i) {
            registry.GetComponent<ECS::CardPlacementComponent>(hand[i]).index = i - 1;
        }
        return entity;
    }

    //----------------------------------------------------------------------------
    //! 手札の枚数を返します。
    //----------------------------------------------------------------------------
    size_t BoardView::HandSize(size_t player) {
        return CollectZone(m_factory.Registry(), ECS::CardZone::Hand, player).size();
    }

    //----------------------------------------------------------------------------
    //! 手札の i 枚目のカードを返します。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity BoardView::HandCard(size_t player, size_t index) {
        return CollectZone(m_factory.Registry(), ECS::CardZone::Hand, player)[index];
    }

    //----------------------------------------------------------------------------
    //! 場の列のカードを返します。
    //----------------------------------------------------------------------------
    std::vector<Tsukino::ECS::Entity> BoardView::Row() {
        return CollectZone(m_factory.Registry(), ECS::CardZone::Row);
    }

    //----------------------------------------------------------------------------
    //! 手札のカードの、並びの中での姿勢を計算します。
    //----------------------------------------------------------------------------
    void BoardView::HandPose(size_t player, size_t index, hlslpp::float3& position, hlslpp::quaternion& rotation, float& scale) {
        const std::vector<Tsukino::ECS::Entity> hand = CollectZone(m_factory.Registry(), ECS::CardZone::Hand, player);
        Board::HandPose(State(), player, index, hand.size(), hand[index], position, rotation, scale);
    }

    //----------------------------------------------------------------------------
    //! カードを捨て札の山に送ります。
    //----------------------------------------------------------------------------
    void BoardView::SendToDiscard(Tsukino::ECS::Entity entity) {
        Tsukino::ECS::Registry& registry = m_factory.Registry();
        const size_t            index    = CollectZone(registry, ECS::CardZone::Discard).size();

        std::uniform_real_distribution<float> unit(-1.0f, 1.0f);
        auto& placement  = Place(registry, entity, ECS::CardZone::Discard, 0, index);
        placement.jitter = hlslpp::float4(unit(m_rng) * 14.0f, unit(m_rng) * 14.0f, unit(m_rng) * 0.35f, 0.0f);

        hlslpp::float3     position;
        hlslpp::quaternion rotation;
        DiscardPose(index, placement.jitter, position, rotation);
        ECS::CardMotion::SetTarget(registry, entity, position, rotation);
        ECS::CardMotion::Fly(registry, entity, 0.55f, 90.0f);
    }

    //----------------------------------------------------------------------------
    //! 山札の残り枚数に合わせて、山の厚みを変えます。
    //----------------------------------------------------------------------------
    void BoardView::SetDeckCount(size_t remaining) {
        std::vector<Tsukino::ECS::Entity>& covers = State().deckCovers;
        const size_t                       wanted = remaining == 0 ? 0 : std::min(kMaxCovers, (remaining + 4) / 5);
        while(covers.size() > wanted) {
            m_factory.Destroy(covers.back());
            covers.pop_back();
        }
        std::uniform_real_distribution<float> unit(-1.0f, 1.0f);
        while(covers.size() < wanted) {
            const float          y        = kTableSurfaceY + static_cast<float>(covers.size()) * kCoverThickness;
            const hlslpp::float3 position = kDeckPosition + hlslpp::float3(unit(m_rng) * 1.5f, y, unit(m_rng) * 1.5f);
            covers.push_back(m_factory.CreateCover(position, hlslpp::quaternion::rotation_y(unit(m_rng) * 0.05f)));
        }
    }

}    // namespace ChronoSort::Board
