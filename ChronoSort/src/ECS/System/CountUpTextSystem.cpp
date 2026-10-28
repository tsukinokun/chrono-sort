//----------------------------------------------------------------------------
//! @file   CountUpTextSystem.cpp
//! @brief  数字の文字を数え上げるシステムの実装
//----------------------------------------------------------------------------
#include <ChronoSort/ECS/System/CountUpTextSystem.hpp>

#include <ChronoSort/ECS/Component/CountUpTextComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>

#include <algorithm>
#include <string>

namespace ChronoSort::ECS {

    //----------------------------------------------------------------------------
    //! 数え上げを進めます。
    //----------------------------------------------------------------------------
    void CountUpTextSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        registry.View<CountUpTextComponent, Tsukino::BuiltIn::ECS::FontComponent>().each(
            [deltaTime](CountUpTextComponent& count, Tsukino::BuiltIn::ECS::FontComponent& font) {
                if(count.duration > 0.0f) {
                    // 時間で数え上げる：終わりほどゆっくり
                    count.elapsed += deltaTime;
                    const float t     = std::clamp((count.elapsed - count.delay) / count.duration, 0.0f, 1.0f);
                    const float eased = 1.0f - (1.0f - t) * (1.0f - t);
                    count.shown       = count.target * eased;
                } else if(count.shown < count.target) {
                    // 目標を追いかける：離れているほど速い
                    count.shown = std::min(count.target, count.shown + std::max(count.minSpeed, (count.target - count.shown) * count.chase) * deltaTime);
                }
                const long long value = static_cast<long long>(count.shown);
                if(value != count.written) {
                    count.written = value;
                    font.text     = std::to_wstring(value);
                }
            });
    }

}    // namespace ChronoSort::ECS
