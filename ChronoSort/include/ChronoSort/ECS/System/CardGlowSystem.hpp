//----------------------------------------------------------------------------
//! @file   CardGlowSystem.hpp
//! @brief  カードの発光を更新するシステム
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Game/TuningTypes.hpp>

#include <Tsukino/Core/ECS/System/ISystem.hpp>

#include <hlsl++.h>

namespace ChronoSort::ECS {

    //! CardGlowComponent を減衰させ、MaterialPropertyBlockComponent の emissive へ反映します。
    class CardGlowSystem : public Tsukino::ECS::ISystem {
    public:

        //! 発光を更新します。
        //! @param  [in,out] registry  レジストリ。
        //! @param  [in]     deltaTime 経過時間（秒）。
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

    //! カードを光らせます。既に光っていれば上書きします。
    //! @param  [in,out] registry  レジストリ。
    //! @param  [in]     entity    カード。
    //! @param  [in]     color     発光色。
    //! @param  [in]     intensity 強さ。
    //! @param  [in]     decay     1秒あたりに減る強さ。
    void Glow(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const hlslpp::float3& color, float intensity, float decay);

    //! カードを脈打つように光らせ続けます。止めるには StopGlow を呼びます。
    //! @param  [in,out] registry  レジストリ。
    //! @param  [in]     entity    カード。
    //! @param  [in]     color     発光色。
    //! @param  [in]     intensity 強さの最大。
    //! @param  [in]     speed     脈打つ速さ（1秒あたりのラジアン）。
    void PulseGlow(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const hlslpp::float3& color, float intensity, float speed);

    //! 調整値の組で、カードを光らせます（rate は1秒あたりに減る強さ）。
    //! @param  [in,out] registry レジストリ。
    //! @param  [in]     entity   カード。
    //! @param  [in]     glow     光り方。
    inline void Glow(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const Game::GlowSpec& glow) {
        Glow(registry, entity, glow.color, glow.intensity, glow.rate);
    }

    //! 調整値の組で、カードを脈打つように光らせ続けます（rate は脈打つ速さ）。
    //! @param  [in,out] registry レジストリ。
    //! @param  [in]     entity   カード。
    //! @param  [in]     glow     光り方。
    inline void PulseGlow(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const Game::GlowSpec& glow) {
        PulseGlow(registry, entity, glow.color, glow.intensity, glow.rate);
    }

    //! カードの発光を止めます。
    //! @param  [in,out] registry レジストリ。
    //! @param  [in]     entity   カード。
    void StopGlow(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity);

}    // namespace ChronoSort::ECS
