//----------------------------------------------------------------------------
//! @file   ParticleEmitter.hpp
//! @brief  演出用の粒子を一度に撒く
//! @detail 「置いたときの砂埃（dust）」「正解の金の火花（gold）」など、撒き方は Particles.json に名前で書きます。
//!         粒子を作るのはエンジンの BurstEmitter、動かすのは BurstParticleSystem です。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>

#include <hlsl++.h>

#include <random>
#include <string>
#include <unordered_map>

namespace Tsukino::ECS {
    class Scene;
}    // namespace Tsukino::ECS

namespace Tsukino::Asset {
    class AssetManager;
}    // namespace Tsukino::Asset

namespace ChronoSort::Effects {

    //! 演出用の粒子を撒きます。
    class ParticleEmitter {
    public:

        //! 撒く先のシーンとアセット管理を受け取り、撒き方に書かれたテクスチャを先に読み込みます。
        //! @param  [in] scene        粒子を作るシーン。
        //! @param  [in] assetManager アセット管理。
        ParticleEmitter(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager);

        //! 名前の撒き方で粒子を撒きます（Particles.json に無い名前なら警告を出して何もしない）。
        //! @param  [in] effect   撒き方の名前（"dust" など）。
        //! @param  [in] position 撒く中心。
        void Emit(const std::string& effect, const hlslpp::float3& position);

    private:

        //! 粒子のテクスチャを返します（初めてなら読み込む）。
        //! @param  [in] file ChronoSort/Assets/Textures/Fx/ からのパス。
        //! @return テクスチャ。
        Tsukino::Asset::AssetHandle Texture(const std::string& file);

        Tsukino::ECS::Scene&                                         m_scene;           // 粒子を作るシーン
        Tsukino::Asset::AssetManager&                                m_assetManager;    // アセット管理
        std::mt19937                                                 m_rng;             // ばらつき用の乱数
        std::unordered_map<std::string, Tsukino::Asset::AssetHandle> m_textures;        // 読み込んだテクスチャ
    };

}    // namespace ChronoSort::Effects
