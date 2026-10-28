//----------------------------------------------------------------------------
//! @file   ChoiceTagSystem.cpp
//! @brief  選択肢の札の見た目を、選ばれ方に合わせるシステムの実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/System/ChoiceTagSystem.hpp>

#include <ChronoSort/ECS/Component/ChoiceTagComponent.hpp>
#include <ChronoSort/Game/GameServices.hpp>

#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIToggleComponent.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UITweenUtility.hpp>

#include <cmath>

namespace ChronoSort::ECS {

    namespace {
        namespace BuiltIn = Tsukino::BuiltIn::ECS;
    }    // namespace

    //----------------------------------------------------------------------------
    //! 札の見た目を更新します。
    //----------------------------------------------------------------------------
    void ChoiceTagSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        bool chosen = false;
        registry.View<ChoiceTagComponent, BuiltIn::UIToggleComponent>().each(
            [&](Tsukino::ECS::Entity entity, ChoiceTagComponent& tag, const BuiltIn::UIToggleComponent& toggle) {
                if(tag.painted && !toggle.changed) {
                    return;
                }
                const bool created = !tag.painted;
                tag.painted        = true;
                if(auto* font = registry.try_get<BuiltIn::FontComponent>(tag.text)) {
                    font->color = toggle.isOn ? tag.chosenColor : tag.otherColor;
                }
                // 選び直された札を弾ませる（作った直後は punchOnCreate のときだけ）
                if(toggle.isOn && tag.punch > 0.0f && (toggle.changed || (created && tag.punchOnCreate))) {
                    BuiltIn::UITweenUtility::Punch(registry, entity, tag.punch);
                }
                chosen = chosen || (toggle.changed && toggle.isOn);
            });
        if(chosen) {
            Game::Services().Sounds().Play("ui.choose");
        }

        // 選ばれた札ほど持ち上げて大きく、ほかは少し沈めて小さく
        const float rate = deltaTime;
        registry.View<ChoiceLiftComponent, BuiltIn::TransformComponent>().each(
            [&](ChoiceLiftComponent& lift, BuiltIn::TransformComponent& transform) {
                const auto* toggle = registry.try_get<BuiltIn::UIToggleComponent>(lift.tag);
                const float target = (toggle != nullptr && toggle->isOn) ? 1.0f : 0.0f;
                lift.lift += (target - lift.lift) * (1.0f - std::exp(-lift.rate * rate));

                transform.position = hlslpp::float3(lift.base + hlslpp::float2(0.0f, lift.restOffset - lift.liftOffset * lift.lift), 0.0f);
                const float scale  = lift.restScale + lift.liftScale * lift.lift;
                transform.scale    = hlslpp::float3(scale, scale, 1.0f);
                transform.dirty    = true;
            });
    }

}    // namespace ChronoSort::ECS
