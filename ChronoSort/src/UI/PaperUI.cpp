//----------------------------------------------------------------------------
//! @file   PaperUI.cpp
//! @brief  「机の上の紙片」でできた画面 UI の部品の実装
//----------------------------------------------------------------------------
#include <ChronoSort/UI/PaperUI.hpp>

#include <ChronoSort/Game/GameServices.hpp>
#include <ChronoSort/UI/TweenTags.hpp>
#include <ChronoSort/Util/AssetPaths.hpp>

#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/LifetimeComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/PointerTargetComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIButtonFeedbackComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIButtonSoundComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIRectComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Transform/TransformUtility.hpp>
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
        return m_assetManager.Load(Tsukino::Core::Path(Util::AssetPaths::UITexture(texture)));
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
            sway.tag      = Tag(TweenTag::Sway);
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
            bob.tag      = Tag(TweenTag::Bob);
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

        // カーソルが新しく紙に乗ったら、紙が擦れる小さな音（押したときの音は、何が起きるかで画面ごとに鳴らし分ける）
        Game::SoundBank& sounds = Game::Services().Sounds();
        float            volume = 1.0f;
        if(const auto hover = sounds.ResolveCue("ui.hover", volume)) {
            auto& sound       = registry.AddComponent<BuiltIn::UIButtonSoundComponent>(entity);
            sound.hoverSound  = *hover;
            sound.hoverVolume = volume;
            sound.bus         = Game::SoundBank::kEffectsBus;
        }
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
        BuiltIn::UITweenUtility::SlideIn(m_scene.GetRegistry(), entity, from, delay, seconds);
    }

    //----------------------------------------------------------------------------
    //! 画面外へ滑り出させ、透明にします。
    //----------------------------------------------------------------------------
    void PaperUI::SlideOut(Tsukino::ECS::Entity entity, const hlslpp::float2& to, float delay, float seconds) {
        BuiltIn::UITweenUtility::SlideOut(m_scene.GetRegistry(), entity, to, delay, seconds);
    }

    //----------------------------------------------------------------------------
    //! 弾ませます。
    //----------------------------------------------------------------------------
    void PaperUI::Punch(Tsukino::ECS::Entity entity, float amount) {
        BuiltIn::UITweenUtility::Punch(m_scene.GetRegistry(), entity, amount);
    }

    //----------------------------------------------------------------------------
    //! 揺れと上下の漂いを止めます。
    //----------------------------------------------------------------------------
    void PaperUI::StopIdle(Tsukino::ECS::Entity entity) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        BuiltIn::UITweenUtility::Stop(registry, entity, Tag(TweenTag::Sway));
        BuiltIn::UITweenUtility::Stop(registry, entity, Tag(TweenTag::Bob));
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
        BuiltIn::TransformUtility::DestroyHierarchy(m_scene.GetRegistry(), entity);
    }

    //----------------------------------------------------------------------------
    //! しばらく置いてから、子の UI ごと破棄させます。
    //----------------------------------------------------------------------------
    void PaperUI::DestroyAfter(Tsukino::ECS::Entity entity, float seconds) {
        if(entity == entt::null) {
            return;
        }
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        auto*                   lifetime = registry.try_get<BuiltIn::LifetimeComponent>(entity);
        if(lifetime == nullptr) {
            lifetime = &registry.AddComponent<BuiltIn::LifetimeComponent>(entity);
        }
        lifetime->remaining = seconds;
    }

}    // namespace ChronoSort::UI
