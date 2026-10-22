//----------------------------------------------------------------------------
//! @file   ParticleEmitter.cpp
//! @brief  演出用の粒子を一度に撒く処理の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Effects/ParticleEmitter.hpp>

#include <ChronoSort/ECS/Component/ParticleComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>

#include <cmath>

namespace ChronoSort::Effects {

    namespace {
        constexpr float kTwoPi = 6.2831853f;
    }    // namespace

    //----------------------------------------------------------------------------
    //! 撒く先のシーンとアセット管理を受け取ります。
    //----------------------------------------------------------------------------
    ParticleEmitter::ParticleEmitter(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager)
        : m_scene(scene)
        , m_assetManager(assetManager)
        , m_rng(std::random_device{}()) {
        m_soft     = assetManager.Load(Tsukino::Core::Path("ChronoSort/Assets/Textures/Fx/soft.png"));
        m_spark    = assetManager.Load(Tsukino::Core::Path("ChronoSort/Assets/Textures/Fx/spark.png"));
        m_ink      = assetManager.Load(Tsukino::Core::Path("ChronoSort/Assets/Textures/Fx/ink.png"));
        m_confetti = assetManager.Load(Tsukino::Core::Path("ChronoSort/Assets/Textures/Fx/confetti.png"));
    }

    //----------------------------------------------------------------------------
    //! 置いたときの砂埃を撒きます。
    //----------------------------------------------------------------------------
    void ParticleEmitter::Dust(const hlslpp::float3& position) {
        BurstParams dust;
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
        Burst(m_soft, dust, position);
    }

    //----------------------------------------------------------------------------
    //! 正解のときの金の火花と紙吹雪を撒きます。
    //----------------------------------------------------------------------------
    void ParticleEmitter::GoldBurst(const hlslpp::float3& position) {
        BurstParams sparks;
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
        Burst(m_spark, sparks, position);

        BurstParams confetti;
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
        Burst(m_confetti, confetti, position);
    }

    //----------------------------------------------------------------------------
    //! 不正解のときの墨の飛沫を撒きます。
    //----------------------------------------------------------------------------
    void ParticleEmitter::InkSplash(const hlslpp::float3& position) {
        BurstParams ink;
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
        Burst(m_ink, ink, position);
    }

    //----------------------------------------------------------------------------
    //! 飛んでいくカードの軌跡に、小さな光の粒を1つ残します。
    //----------------------------------------------------------------------------
    void ParticleEmitter::TrailMote(const hlslpp::float3& position) {
        BurstParams mote;
        mote.count        = 1;
        mote.speedMin     = 5.0f;
        mote.speedMax     = 20.0f;
        mote.upwardBias   = 1.0f;
        mote.lifeMin      = 0.3f;
        mote.lifeMax      = 0.6f;
        mote.sizeMin      = 5.0f;
        mote.sizeMax      = 9.0f;
        mote.gravity      = -30.0f;
        mote.radius       = 12.0f;
        mote.colorA       = hlslpp::float4(1.6f, 1.2f, 0.6f, 0.8f);
        mote.colorB       = hlslpp::float4(1.2f, 1.0f, 0.7f, 0.6f);
        Burst(m_soft, mote, position);
    }

    //----------------------------------------------------------------------------
    //! 撒き方を指定して粒子を撒きます。
    //----------------------------------------------------------------------------
    void ParticleEmitter::Burst(Tsukino::Asset::AssetHandle texture, const BurstParams& params, const hlslpp::float3& position) {
        std::uniform_real_distribution<float> unit(0.0f, 1.0f);
        auto range = [&](float a, float b) { return a + (b - a) * unit(m_rng); };

        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        for(int i = 0; i < params.count; ++i) {
            const float angle  = range(0.0f, kTwoPi);
            const float radius = range(0.0f, params.radius);
            const float speed  = range(params.speedMin, params.speedMax);

            hlslpp::float3 direction(std::cos(angle) * params.spread, range(0.3f, 1.0f) * params.upwardBias, std::sin(angle) * params.spread);
            direction = hlslpp::normalize(direction + hlslpp::float3(0.0f, 0.001f, 0.0f));

            Tsukino::ECS::Entity entity = m_scene.CreateEntity();
            auto&                transform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(entity);
            transform.position = position + hlslpp::float3(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius);
            transform.scale    = hlslpp::float3(0.0f, 0.0f, 1.0f);

            auto& sprite         = registry.AddComponent<Tsukino::BuiltIn::ECS::SpriteComponent>(entity);
            sprite.textureHandle = texture;
            sprite.space         = Tsukino::BuiltIn::ECS::SpriteSpace::World;
            sprite.blendMode     = params.additive ? Tsukino::BuiltIn::ECS::SpriteBlendMode::Additive : Tsukino::BuiltIn::ECS::SpriteBlendMode::Alpha;
            sprite.raycastTarget = false;

            auto& particle      = registry.AddComponent<ECS::ParticleComponent>(entity);
            particle.velocity   = direction * speed;
            particle.gravity    = params.gravity;
            particle.drag       = params.drag;
            particle.age        = -range(0.0f, params.delayMax);
            particle.life       = range(params.lifeMin, params.lifeMax);
            particle.startSize  = range(params.sizeMin, params.sizeMax);
            particle.endSize    = particle.startSize * params.endSizeScale;
            particle.startColor = hlslpp::lerp(params.colorA, params.colorB, hlslpp::float4(unit(m_rng)));
            particle.endColor   = hlslpp::float4(particle.startColor.xyz, params.endAlpha);
            sprite.tintColor    = particle.startColor;
        }
    }

}    // namespace ChronoSort::Effects
