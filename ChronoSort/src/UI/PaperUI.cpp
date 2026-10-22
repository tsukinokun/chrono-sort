//----------------------------------------------------------------------------
//! @file   PaperUI.cpp
//! @brief  「机の上の紙片」でできた画面 UI の部品の実装
//----------------------------------------------------------------------------
#include <ChronoSort/UI/PaperUI.hpp>

#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/PointerTargetComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIButtonFeedbackComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIRectComponent.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UITweenUtility.hpp>
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>

#include <vector>

namespace ChronoSort::UI {

    namespace {
        namespace BuiltIn = Tsukino::BuiltIn::ECS;
        using Tsukino::Core::Math::Easing::EaseType;

        constexpr float kDegToRad = 3.14159265f / 180.0f;

        // トゥイーンの種類の目印（同じ目印を再生し直すと置き換わる）
        constexpr std::uint32_t kTagSway    = 1;
        constexpr std::uint32_t kTagBob     = 2;
        constexpr std::uint32_t kTagSlideX  = 3;
        constexpr std::uint32_t kTagSlideY  = 4;
        constexpr std::uint32_t kTagFade    = 5;
        constexpr std::uint32_t kTagPunch   = 6;
    }    // namespace

    //----------------------------------------------------------------------------
    //! 作り先のシーンとアセット管理を受け取ります。
    //----------------------------------------------------------------------------
    PaperUI::PaperUI(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager)
        : m_scene(scene)
        , m_assetManager(assetManager)
        , m_rng(std::random_device{}()) {
    }

    //----------------------------------------------------------------------------
    //! テクスチャを読み込みます。
    //----------------------------------------------------------------------------
    Tsukino::Asset::AssetHandle PaperUI::Texture(const std::string& texture) {
        return m_assetManager.Load(Tsukino::Core::Path("ChronoSort/Assets/Textures/UI/" + texture));
    }

    //----------------------------------------------------------------------------
    //! -1〜1 の乱数を返します。
    //----------------------------------------------------------------------------
    float PaperUI::Jitter() {
        return std::uniform_real_distribution<float>(-1.0f, 1.0f)(m_rng);
    }

    //----------------------------------------------------------------------------
    //! 紙片を1枚置きます。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity PaperUI::Paper(const PaperSpec& spec) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        Tsukino::ECS::Entity    entity   = m_scene.CreateEntity();

        auto& transform    = registry.AddComponent<BuiltIn::TransformComponent>(entity);
        transform.position = hlslpp::float3(spec.center, 0.0f);
        transform.rotation = hlslpp::quaternion::rotation_z(spec.angleDegrees * kDegToRad);
        transform.parent   = spec.parent;

        auto& sprite         = registry.AddComponent<BuiltIn::SpriteComponent>(entity);
        sprite.textureHandle = Texture(spec.texture);
        sprite.sortOrder     = spec.sortOrder;
        if(float(spec.size.x) > 0.0f) {
            auto& rect = registry.AddComponent<BuiltIn::UIRectComponent>(entity);
            rect.size  = spec.size;
        }

        // 紙片ごとに周期と位相をずらして揺らす（全部が揃って動くと機械的に見える）
        if(spec.swayDegrees > 0.0f) {
            BuiltIn::UITween sway;
            sway.property = BuiltIn::UITweenProperty::Rotation;
            sway.from     = -spec.swayDegrees * kDegToRad;
            sway.to       = spec.swayDegrees * kDegToRad;
            sway.duration = spec.swaySeconds * (0.85f + 0.3f * (Jitter() * 0.5f + 0.5f)) * 0.5f;
            sway.delay    = (Jitter() * 0.5f + 0.5f) * spec.swaySeconds * 0.5f;
            sway.ease     = EaseType::InOutSine;
            sway.loop     = BuiltIn::UITweenLoop::PingPong;
            sway.tag      = kTagSway;
            BuiltIn::UITweenUtility::Play(registry, entity, sway);
        }
        if(spec.bobPixels > 0.0f) {
            BuiltIn::UITween bob;
            bob.property = BuiltIn::UITweenProperty::OffsetY;
            bob.from     = -spec.bobPixels;
            bob.to       = spec.bobPixels;
            bob.duration = spec.swaySeconds * 0.7f * (0.85f + 0.3f * (Jitter() * 0.5f + 0.5f));
            bob.delay    = (Jitter() * 0.5f + 0.5f) * spec.swaySeconds * 0.4f;
            bob.ease     = EaseType::InOutSine;
            bob.loop     = BuiltIn::UITweenLoop::PingPong;
            bob.tag      = kTagBob;
            BuiltIn::UITweenUtility::Play(registry, entity, bob);
        }
        return entity;
    }

    //----------------------------------------------------------------------------
    //! 文字を置きます。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity PaperUI::Text(const std::wstring& text, const hlslpp::float2& position, float scale, const hlslpp::float4& color, int sortOrder,
                                       Tsukino::ECS::Entity parent, bool centered) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        Tsukino::ECS::Entity    entity   = m_scene.CreateEntity();

        auto& transform    = registry.AddComponent<BuiltIn::TransformComponent>(entity);
        transform.position = hlslpp::float3(position, 0.0f);
        transform.parent   = parent;

        auto& font           = registry.AddComponent<BuiltIn::FontComponent>(entity);
        font.text            = text;
        font.scale           = scale;
        font.color           = color;
        font.sortOrder       = sortOrder;
        font.horizontalAlign = centered ? BuiltIn::HorizontalAlign::Center : BuiltIn::HorizontalAlign::Left;
        font.verticalAlign   = BuiltIn::VerticalAlign::Middle;
        return entity;
    }

    //----------------------------------------------------------------------------
    //! 紙片をボタンにします。
    //----------------------------------------------------------------------------
    void PaperUI::MakeButton(Tsukino::ECS::Entity entity) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        registry.AddComponent<BuiltIn::PointerTargetComponent>(entity);
        auto& feedback      = registry.AddComponent<BuiltIn::UIButtonFeedbackComponent>(entity);
        feedback.hoverScale = 1.08f;
        feedback.pressScale = 0.93f;
        feedback.clickPunch = 0.14f;
    }

    //----------------------------------------------------------------------------
    //! 紙片が押されたかを返します。
    //----------------------------------------------------------------------------
    bool PaperUI::Clicked(Tsukino::ECS::Entity entity) {
        auto* target = m_scene.GetRegistry().try_get<BuiltIn::PointerTargetComponent>(entity);
        return target != nullptr && target->clicked;
    }

    //----------------------------------------------------------------------------
    //! 紙片にカーソルが重なっているかを返します。
    //----------------------------------------------------------------------------
    bool PaperUI::Hovered(Tsukino::ECS::Entity entity) {
        auto* target = m_scene.GetRegistry().try_get<BuiltIn::PointerTargetComponent>(entity);
        return target != nullptr && target->hovered;
    }

    //----------------------------------------------------------------------------
    //! 画面外から滑り込ませます。
    //----------------------------------------------------------------------------
    void PaperUI::SlideIn(Tsukino::ECS::Entity entity, const hlslpp::float2& from, float delay, float seconds) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        BuiltIn::UITween        tween;
        tween.duration  = seconds;
        tween.delay     = delay;
        tween.ease      = EaseType::OutBack;
        tween.holdAtEnd = true;

        tween.property = BuiltIn::UITweenProperty::OffsetX;
        tween.from     = from.x;
        tween.to       = 0.0f;
        tween.tag      = kTagSlideX;
        BuiltIn::UITweenUtility::Play(registry, entity, tween);

        tween.property = BuiltIn::UITweenProperty::OffsetY;
        tween.from     = from.y;
        tween.tag      = kTagSlideY;
        BuiltIn::UITweenUtility::Play(registry, entity, tween);

        tween.property = BuiltIn::UITweenProperty::Alpha;
        tween.from     = 0.0f;
        tween.to       = 1.0f;
        tween.ease     = EaseType::OutCubic;
        tween.tag      = kTagFade;
        BuiltIn::UITweenUtility::Play(registry, entity, tween);
    }

    //----------------------------------------------------------------------------
    //! 画面外へ滑り出させ、透明にします。
    //----------------------------------------------------------------------------
    void PaperUI::SlideOut(Tsukino::ECS::Entity entity, const hlslpp::float2& to, float delay, float seconds) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        BuiltIn::UITween        tween;
        tween.duration  = seconds;
        tween.delay     = delay;
        tween.ease      = EaseType::InCubic;
        tween.holdAtEnd = true;

        tween.property = BuiltIn::UITweenProperty::OffsetX;
        tween.from     = 0.0f;
        tween.to       = to.x;
        tween.tag      = kTagSlideX;
        BuiltIn::UITweenUtility::Play(registry, entity, tween);

        tween.property = BuiltIn::UITweenProperty::OffsetY;
        tween.to       = to.y;
        tween.tag      = kTagSlideY;
        BuiltIn::UITweenUtility::Play(registry, entity, tween);

        tween.property = BuiltIn::UITweenProperty::Alpha;
        tween.from     = 1.0f;
        tween.to       = 0.0f;
        tween.tag      = kTagFade;
        BuiltIn::UITweenUtility::Play(registry, entity, tween);
    }

    //----------------------------------------------------------------------------
    //! 弾ませます。
    //----------------------------------------------------------------------------
    void PaperUI::Punch(Tsukino::ECS::Entity entity, float amount) {
        BuiltIn::UITween punch;
        punch.property = BuiltIn::UITweenProperty::Scale;
        punch.from     = 1.0f;
        punch.to       = 1.0f + amount;
        punch.duration = 0.45f;
        punch.ease     = EaseType::Punch;
        punch.tag      = kTagPunch;
        BuiltIn::UITweenUtility::Play(m_scene.GetRegistry(), entity, punch);
    }

    //----------------------------------------------------------------------------
    //! 文字列を書き換えます。
    //----------------------------------------------------------------------------
    void PaperUI::SetText(Tsukino::ECS::Entity entity, const std::wstring& text) {
        if(auto* font = m_scene.GetRegistry().try_get<BuiltIn::FontComponent>(entity)) {
            font->text = text;
        }
    }

    //----------------------------------------------------------------------------
    //! スプライトのテクスチャを差し替えます。
    //----------------------------------------------------------------------------
    void PaperUI::SetTexture(Tsukino::ECS::Entity entity, const std::string& texture) {
        if(auto* sprite = m_scene.GetRegistry().try_get<BuiltIn::SpriteComponent>(entity)) {
            sprite->textureHandle = Texture(texture);
        }
    }

    //----------------------------------------------------------------------------
    //! エンティティと、その子の UI をまとめて破棄します。
    //----------------------------------------------------------------------------
    void PaperUI::Destroy(Tsukino::ECS::Entity entity) {
        if(entity == entt::null) {
            return;
        }
        Tsukino::ECS::Registry&           registry = m_scene.GetRegistry();
        std::vector<Tsukino::ECS::Entity> pending  = {entity};
        // 子は親を指しているだけなので、親から辿れない。全体を見て子孫を集める
        for(size_t i = 0; i < pending.size(); ++i) {
            const Tsukino::ECS::Entity current = pending[i];
            registry.View<BuiltIn::TransformComponent>().each([&](Tsukino::ECS::Entity child, const BuiltIn::TransformComponent& transform) {
                if(static_cast<Tsukino::ECS::Entity>(transform.parent) == current) {
                    pending.push_back(child);
                }
            });
        }
        for(Tsukino::ECS::Entity e : pending) {
            registry.QueueDestroy(e);
        }
    }

}    // namespace ChronoSort::UI
