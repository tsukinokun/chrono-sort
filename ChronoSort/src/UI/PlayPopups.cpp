//----------------------------------------------------------------------------
//! @file   PlayPopups.cpp
//! @brief  判定したカードの上に出す朱印と得点の実装
//----------------------------------------------------------------------------
#include <ChronoSort/UI/PlayPopups.hpp>

#include <ChronoSort/Match/MatchEvents.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/WorldAnchorComponent.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UITweenUtility.hpp>
#include <Tsukino/Core/ECS/Event/EventBus.hpp>
#include <Tsukino/Core/Math/Color.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>

#include <cstdint>
#include <string>

namespace ChronoSort::UI {

    namespace {
        namespace BuiltIn = Tsukino::BuiltIn::ECS;
        namespace Color   = Tsukino::Core::Math::Color;
        using Tsukino::Core::Math::Easing::EaseType;

        constexpr float kDegToRad      = 3.14159265f / 180.0f;
        constexpr int   kLayerAnchored = 40;    // カードに貼り付く UI の描画順

        //! トゥイーンを1本再生します（朱印と得点の動きは本数が多いので、組み立てをまとめる）。
        void Tween(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, BuiltIn::UITweenProperty property, float from, float to, float delay,
                   float duration, EaseType ease, std::uint32_t tag) {
            BuiltIn::UITween tween;
            tween.property  = property;
            tween.from      = from;
            tween.to        = to;
            tween.delay     = delay;
            tween.duration  = duration;
            tween.ease      = ease;
            tween.holdAtEnd = true;
            tween.tag       = tag;
            BuiltIn::UITweenUtility::Play(registry, entity, tween);
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! 作り先のシーン・アセット管理・表示する対局を受け取り、出来事の購読を始めます。
    //----------------------------------------------------------------------------
    PlayPopups::PlayPopups(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager, const Rules::Match& match)
        : m_scene(scene)
        , m_ui(scene, assetManager)
        , m_match(match) {
        m_connections.push_back(scene.GetEventBus().Subscribe<Match::PlayJudgedEvent>([this](const Match::PlayJudgedEvent& e) {
            ShowStamp(e.card, e.result.correct);
            if(!e.result.correct) {
                return;
            }
            // ソロは得点、対戦は僅差のときだけ
            if(m_match.Config().mode == Rules::MatchMode::Solo) {
                ShowScorePopup(e.card, e.result.scoreGained, e.result.closeCall, e.result.streak);
            } else if(e.result.closeCall) {
                ShowScorePopup(e.card, 0, true, e.result.streak);
            }
        }));
    }

    //----------------------------------------------------------------------------
    //! カードに貼り付く UI を作ります。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity PlayPopups::CreateAnchor(Tsukino::ECS::Entity card, const hlslpp::float3& worldOffset) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        Tsukino::ECS::Entity    entity   = m_scene.CreateEntity();
        registry.AddComponent<BuiltIn::TransformComponent>(entity);
        auto& anchor       = registry.AddComponent<BuiltIn::WorldAnchorComponent>(entity);
        anchor.target      = card;
        anchor.worldOffset = worldOffset;
        return entity;
    }

    //----------------------------------------------------------------------------
    //! カードの上に判定の朱印を押します。
    //----------------------------------------------------------------------------
    void PlayPopups::ShowStamp(Tsukino::ECS::Entity card, bool correct) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        const auto              anchor   = CreateAnchor(card, hlslpp::float3(0.0f, 20.0f, 0.0f));

        PaperSpec spec;
        spec.texture      = correct ? "stamp_correct.png" : "stamp_wrong.png";
        spec.size         = hlslpp::float2(150.0f, 150.0f);
        spec.angleDegrees = m_ui.Jitter() * 14.0f;
        spec.swayDegrees  = 0.0f;
        spec.bobPixels    = 0.0f;
        spec.sortOrder    = kLayerAnchored + 5;
        spec.parent       = anchor;
        const auto stamp  = m_ui.Paper(spec);

        using Property = BuiltIn::UITweenProperty;
        // 上から押し付ける：大きく薄い状態から一気に縮んで濃くなる
        Tween(registry, stamp, Property::Scale, 2.4f, 1.0f, 0.0f, 0.22f, EaseType::OutBack, 30);
        Tween(registry, stamp, Property::Alpha, 0.0f, 1.0f, 0.0f, 0.15f, EaseType::OutCubic, 31);
        // 押した後も止めない：押し付けた反動で揺れ戻り、インクがじわっと滲む
        Tween(registry, stamp, Property::Rotation, 0.0f, 5.0f * kDegToRad, 0.2f, 0.6f, EaseType::Punch, 34);
        Tween(registry, stamp, Property::Scale, 1.0f, 1.05f, 0.22f, 0.88f, EaseType::OutQuad, 35);
        // 押した後は、ふわっと広がりながら消える
        Tween(registry, stamp, Property::Scale, 1.0f, 1.6f, 1.1f, 0.4f, EaseType::OutCubic, 33);
        Tween(registry, anchor, Property::Alpha, 1.0f, 0.0f, 1.1f, 0.4f, EaseType::OutCubic, 32);

        m_ui.DestroyAfter(anchor, 1.6f);
    }

    //----------------------------------------------------------------------------
    //! カードの上に得点を浮き上がらせます。
    //----------------------------------------------------------------------------
    void PlayPopups::ShowScorePopup(Tsukino::ECS::Entity card, int score, bool closeCall, int streak) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        const auto              anchor   = CreateAnchor(card, hlslpp::float3(0.0f, 40.0f, 30.0f));

        float line = 0.0f;
        if(score > 0) {
            m_ui.Text(L"+" + std::to_wstring(score), hlslpp::float2(70.0f, line), 1.1f, Color::SrgbToLinear(hlslpp::float4(1.0f, 0.86f, 0.45f, 1.0f)),
                      kLayerAnchored + 6, anchor);
            line += 36.0f;
        }
        if(closeCall) {
            m_ui.Text(L"僅差！", hlslpp::float2(70.0f, line), 0.7f, Color::SrgbToLinear(hlslpp::float4(1.0f, 0.95f, 0.8f, 1.0f)), kLayerAnchored + 6, anchor);
            line += 28.0f;
        }
        if(streak >= 2) {
            m_ui.Text(std::to_wstring(streak) + L"連続", hlslpp::float2(70.0f, line), 0.7f, Color::SrgbToLinear(hlslpp::float4(1.0f, 0.7f, 0.5f, 1.0f)),
                      kLayerAnchored + 6, anchor);
        }

        using Property = BuiltIn::UITweenProperty;
        Tween(registry, anchor, Property::OffsetY, 10.0f, -60.0f, 0.0f, 1.3f, EaseType::OutCubic, 40);
        Tween(registry, anchor, Property::Scale, 0.4f, 1.0f, 0.0f, 0.3f, EaseType::OutBack, 41);
        Tween(registry, anchor, Property::Alpha, 1.0f, 0.0f, 1.0f, 0.4f, EaseType::OutCubic, 42);

        m_ui.DestroyAfter(anchor, 1.5f);
    }

}    // namespace ChronoSort::UI
