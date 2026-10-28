//----------------------------------------------------------------------------
//! @file   ParticleTuning.hpp
//! @brief  演出用の粒子の撒き方（ChronoSort/Assets/Data/Tuning/Particles.json）
//! @detail 名前（"dust" など）ごとに、テクスチャと撒き方（エンジンの BurstParams）の組を並べます。
//!         1つの名前に複数の組を書くと、同時に撒きます（火花と紙吹雪など）。
//!         JSON に effects を書くと、その名前の一覧で初期値をまるごと置き換えます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Serialization/BurstParamsSerialization.hpp>
#include <Tsukino/Core/IO/CerealOptional.hpp>

#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include <map>
#include <string>
#include <vector>

namespace ChronoSort::Effects {

    //! テクスチャ1種類ぶんの撒き方です。
    struct ParticleLayer {
        std::string                         texture;    // ChronoSort/Assets/Textures/Fx/ からのパス
        Tsukino::BuiltIn::ECS::BurstParams  params;     // 撒き方
    };

    //! 演出用の粒子の撒き方の一覧です。
    struct ParticleTuning {
        std::map<std::string, std::vector<ParticleLayer>> effects = DefaultEffects();    // 名前 → 同時に撒く組

        //! JSON を書く前の撒き方を返します。
        //! @return 名前 → 同時に撒く組。
        static std::map<std::string, std::vector<ParticleLayer>> DefaultEffects() {
            Tsukino::BuiltIn::ECS::BurstParams dust;
            dust.count        = 22;
            dust.speedMin     = 60.0f;
            dust.speedMax     = 140.0f;
            dust.upwardBias   = 0.15f;
            dust.lifeMin      = 0.5f;
            dust.lifeMax      = 0.9f;
            dust.sizeMin      = 10.0f;
            dust.sizeMax      = 22.0f;
            dust.endSizeScale = 2.2f;
            dust.gravity      = -10.0f;
            dust.drag         = 4.0f;
            dust.radius       = 30.0f;
            dust.colorA       = hlslpp::float4(0.55f, 0.45f, 0.33f, 0.45f);
            dust.colorB       = hlslpp::float4(0.75f, 0.65f, 0.50f, 0.35f);
            dust.additive     = false;

            Tsukino::BuiltIn::ECS::BurstParams sparks;
            sparks.count      = 40;
            sparks.speedMin   = 120.0f;
            sparks.speedMax   = 320.0f;
            sparks.upwardBias = 1.4f;
            sparks.lifeMin    = 0.5f;
            sparks.lifeMax    = 1.1f;
            sparks.sizeMin    = 6.0f;
            sparks.sizeMax    = 14.0f;
            sparks.gravity    = 260.0f;
            sparks.drag       = 1.5f;
            sparks.radius     = 20.0f;
            sparks.colorA     = hlslpp::float4(4.0f, 2.8f, 0.9f, 1.0f);    // HDR の金（Bloom で滲む）
            sparks.colorB     = hlslpp::float4(3.0f, 3.0f, 2.2f, 1.0f);

            Tsukino::BuiltIn::ECS::BurstParams confetti;
            confetti.count        = 26;
            confetti.speedMin     = 90.0f;
            confetti.speedMax     = 200.0f;
            confetti.upwardBias   = 2.0f;
            confetti.lifeMin      = 1.0f;
            confetti.lifeMax      = 1.8f;
            confetti.sizeMin      = 6.0f;
            confetti.sizeMax      = 10.0f;
            confetti.endSizeScale = 0.8f;
            confetti.gravity      = 140.0f;
            confetti.drag         = 2.5f;
            confetti.radius       = 25.0f;
            confetti.delayMax     = 0.12f;
            confetti.colorA       = hlslpp::float4(0.95f, 0.85f, 0.55f, 1.0f);
            confetti.colorB       = hlslpp::float4(0.80f, 0.30f, 0.22f, 1.0f);
            confetti.endAlpha     = 0.2f;
            confetti.additive     = false;

            Tsukino::BuiltIn::ECS::BurstParams ink;
            ink.count        = 18;
            ink.speedMin     = 80.0f;
            ink.speedMax     = 220.0f;
            ink.upwardBias   = 0.9f;
            ink.lifeMin      = 0.5f;
            ink.lifeMax      = 0.9f;
            ink.sizeMin      = 8.0f;
            ink.sizeMax      = 18.0f;
            ink.endSizeScale = 0.5f;
            ink.gravity      = 420.0f;
            ink.drag         = 1.0f;
            ink.radius       = 15.0f;
            ink.colorA       = hlslpp::float4(0.08f, 0.05f, 0.06f, 0.95f);
            ink.colorB       = hlslpp::float4(0.35f, 0.04f, 0.04f, 0.9f);
            ink.additive     = false;

            return {
                {"dust", {{"soft.png", dust}}},
                {"gold", {{"spark.png", sparks}, {"confetti.png", confetti}}},
                {"ink", {{"ink.png", ink}}},
            };
        }
    };

    //! ParticleLayer を読み込みます。
    template <class Archive>
    void load(Archive& archive, ParticleLayer& t) {
        Tsukino::IO::LoadOptional(archive, "texture", t.texture);
        Tsukino::IO::LoadOptional(archive, "params", t.params);
    }

    //! ParticleTuning を読み込みます。
    template <class Archive>
    void load(Archive& archive, ParticleTuning& t) {
        Tsukino::IO::LoadOptional(archive, "effects", t.effects);
    }

}    // namespace ChronoSort::Effects
