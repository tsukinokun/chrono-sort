//----------------------------------------------------------------------------
//! @file   ParticleEmitter.cpp
//! @brief  演出用の粒子を一度に撒く処理の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Effects/ParticleEmitter.hpp>

#include <ChronoSort/Game/TuningSet.hpp>
#include <ChronoSort/Util/AssetPaths.hpp>

#include <Tsukino/BuiltIn/ECS/Particle/BurstEmitter.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>

namespace ChronoSort::Effects {

    //----------------------------------------------------------------------------
    //! 撒く先のシーンとアセット管理を受け取り、撒き方に書かれたテクスチャを先に読み込みます。
    //----------------------------------------------------------------------------
    ParticleEmitter::ParticleEmitter(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager)
        : m_scene(scene)
        , m_assetManager(assetManager)
        , m_rng(std::random_device{}()) {
        // 初めて撒くときに読み込むと引っかかるので、先に読んでおく
        for(const auto& [name, layers] : Game::Tuning().Particles().effects) {
            for(const ParticleLayer& layer : layers) {
                Texture(layer.texture);
            }
        }
    }

    //----------------------------------------------------------------------------
    //! 名前の撒き方で粒子を撒きます。
    //----------------------------------------------------------------------------
    void ParticleEmitter::Emit(const std::string& effect, const hlslpp::float3& position) {
        const auto& effects = Game::Tuning().Particles().effects;
        const auto  it      = effects.find(effect);
        if(it == effects.end()) {
            Tsukino::Core::Log::Warn("ParticleEmitter: effect '" + effect + "' is not in Particles.json");
            return;
        }
        for(const ParticleLayer& layer : it->second) {
            Tsukino::BuiltIn::ECS::BurstEmitter::Emit(m_scene, Texture(layer.texture), layer.params, position, m_rng);
        }
    }

    //----------------------------------------------------------------------------
    //! 粒子のテクスチャを返します。
    //----------------------------------------------------------------------------
    Tsukino::Asset::AssetHandle ParticleEmitter::Texture(const std::string& file) {
        if(const auto it = m_textures.find(file); it != m_textures.end()) {
            return it->second;
        }
        const Tsukino::Asset::AssetHandle handle = m_assetManager.Load(Tsukino::Core::Path(Util::AssetPaths::FxTexture(file)));
        m_textures.emplace(file, handle);
        return handle;
    }

}    // namespace ChronoSort::Effects
