//----------------------------------------------------------------------------
//! @file   Widgets.cpp
//! @brief  紙片の UI の部品の実装
//----------------------------------------------------------------------------
#include <ChronoSort/UI/Widgets.hpp>

#include <ChronoSort/ECS/Component/ChoiceTagComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIRectComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIToggleComponent.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UIToggleUtility.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>

#include <algorithm>

namespace ChronoSort::UI {

    namespace {
        namespace BuiltIn = Tsukino::BuiltIn::ECS;
    }    // namespace

    //----------------------------------------------------------------------------
    //! 文字を書いた札のボタンを作ります。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity NoteButton(PaperUI& ui, const PaperSpec& paper, const std::wstring& label, float labelScale, const hlslpp::float2& labelOffset) {
        const Tsukino::ECS::Entity button = ui.Paper(paper);
        ui.Text(label, labelOffset, labelScale, kInkColor, paper.sortOrder + 1, button);
        ui.MakeButton(button);
        return button;
    }

    //----------------------------------------------------------------------------
    //! 選択肢の組を作ります。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity ChoiceGroup(PaperUI& ui) {
        const Tsukino::ECS::Entity group = ui.Scene().CreateEntity();
        ui.Scene().GetRegistry().AddComponent<BuiltIn::UIToggleGroupComponent>(group);
        return group;
    }

    //----------------------------------------------------------------------------
    //! 選択肢の札を1枚作ります。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity ChoiceTag(PaperUI& ui, Tsukino::ECS::Entity group, const ChoiceSpec& spec, bool selected) {
        Tsukino::ECS::Registry&    registry = ui.Scene().GetRegistry();
        const Tsukino::ECS::Entity tag      = ui.Paper(spec.paper);
        const Tsukino::ECS::Entity text     = ui.Text(spec.label, spec.labelOffset, spec.labelScale, spec.otherColor, spec.paper.sortOrder + 1, tag);
        ui.MakeButton(tag);

        auto& toggle = registry.AddComponent<BuiltIn::UIToggleComponent>(tag);
        toggle.group = group;
        toggle.isOn  = selected;

        auto& choice         = registry.AddComponent<ECS::ChoiceTagComponent>(tag);
        choice.text          = text;
        choice.value         = spec.value;
        choice.chosenColor   = spec.chosenColor;
        choice.otherColor    = spec.otherColor;
        choice.punch         = spec.punch;
        choice.punchOnCreate = spec.punchOnCreate;
        return tag;
    }

    //----------------------------------------------------------------------------
    //! 組の中で選ばれている札の値を返します。
    //----------------------------------------------------------------------------
    int SelectedChoice(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity group, int fallback) {
        const Tsukino::ECS::Entity selected = BuiltIn::UIToggleUtility::GetSelected(registry, group);
        const auto*                choice   = (selected != entt::null) ? registry.try_get<ECS::ChoiceTagComponent>(selected) : nullptr;
        return choice != nullptr ? choice->value : fallback;
    }

    //----------------------------------------------------------------------------
    //! 組の選ばれている札が、このフレームにクリックで変わったかを返します。
    //----------------------------------------------------------------------------
    bool ChoiceChanged(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity group) {
        const auto* marker = (group != entt::null) ? registry.try_get<BuiltIn::UIToggleGroupComponent>(group) : nullptr;
        return marker != nullptr && marker->changed;
    }

    //----------------------------------------------------------------------------
    //! 左端から伸び縮みする墨の線を作ります。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity InkLine(PaperUI& ui, Tsukino::ECS::Entity parent, const std::string& texture, const hlslpp::float2& size, int sortOrder,
                                 const hlslpp::float4& tint, float fill) {
        Tsukino::ECS::Registry&    registry = ui.Scene().GetRegistry();
        const Tsukino::ECS::Entity entity   = ui.Scene().CreateEntity();
        registry.AddComponent<BuiltIn::TransformComponent>(entity).parent = parent;

        auto& sprite         = registry.AddComponent<BuiltIn::SpriteComponent>(entity);
        sprite.textureHandle = ui.Texture(texture);
        sprite.sortOrder     = sortOrder;
        sprite.tintColor     = tint;
        sprite.fillMode      = BuiltIn::SpriteFillMode::Horizontal;
        sprite.fillAmount    = std::clamp(fill, 0.0f, 1.0f);

        // 左端を原点に置く（伸びても左端は動かない）
        auto& rect = registry.AddComponent<BuiltIn::UIRectComponent>(entity);
        rect.size  = size;
        rect.pivot = hlslpp::float2(0.0f, 0.5f);
        return entity;
    }

    //----------------------------------------------------------------------------
    //! 墨の線の伸びている割合を変えます。
    //----------------------------------------------------------------------------
    void SetInkLineFill(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity line, float fill) {
        if(auto* sprite = registry.try_get<BuiltIn::SpriteComponent>(line)) {
            sprite->fillAmount = std::clamp(fill, 0.0f, 1.0f);
        }
    }

}    // namespace ChronoSort::UI
