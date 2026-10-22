//----------------------------------------------------------------------------
//! @file   ParticleEmitter.hpp
//! @brief  演出用の粒子を一度に撒く
//! @detail 「置いたときの砂埃」「正解の金の火花」など、場面ごとの撒き方をまとめています。
//!         粒子の動きそのものは ParticleSystem が担当します。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>

#include <hlsl++.h>

#include <random>

namespace Tsukino::ECS {
    class Scene;
}    // namespace Tsukino::ECS

namespace Tsukino::Asset {
    class AssetManager;
}    // namespace Tsukino::Asset

namespace ChronoSort::Effects {

    //! 粒子1種類の撒き方です。値はすべて「最小〜最大」の範囲で乱数を振ります。
    struct BurstParams {
        int            count        = 16;
        float          speedMin     = 40.0f;
        float          speedMax     = 120.0f;
        float          upwardBias   = 0.5f;     // 上向き成分の強さ（0 で水平に広がる）
        float          spread       = 1.0f;     // 水平方向の広がり（0 で真上だけ）
        float          lifeMin      = 0.5f;
        float          lifeMax      = 1.0f;
        float          sizeMin      = 6.0f;
        float          sizeMax      = 12.0f;
        float          endSizeScale = 0.2f;     // 消えるときの大きさの倍率
        float          gravity      = 120.0f;
        float          drag         = 2.0f;
        float          radius       = 10.0f;    // 撒き始めの位置のばらつき
        float          delayMax     = 0.0f;     // 出るタイミングのばらつき（秒）
        hlslpp::float4 colorA       = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);    // 色の候補 A
        hlslpp::float4 colorB       = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);    // 色の候補 B（A との間で乱数）
        float          endAlpha     = 0.0f;
        bool           additive     = true;
    };

    //! 演出用の粒子を撒きます。
    class ParticleEmitter {
    public:

        //! 撒く先のシーンとアセット管理を受け取ります。
        //! @param  [in] scene        粒子を作るシーン。
        //! @param  [in] assetManager アセット管理。
        ParticleEmitter(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager);

        //! 置いたときの砂埃を撒きます。
        //! @param  [in] position カードの中心。
        void Dust(const hlslpp::float3& position);

        //! 正解のときの金の火花と紙吹雪を撒きます。
        //! @param  [in] position カードの中心。
        void GoldBurst(const hlslpp::float3& position);

        //! 不正解のときの墨の飛沫を撒きます。
        //! @param  [in] position カードの中心。
        void InkSplash(const hlslpp::float3& position);

        //! 飛んでいくカードの軌跡に、小さな光の粒を1つ残します。
        //! @param  [in] position 粒を残す位置。
        void TrailMote(const hlslpp::float3& position);

        //! 撒き方を指定して粒子を撒きます。
        //! @param  [in] texture  粒子のテクスチャ。
        //! @param  [in] params   撒き方。
        //! @param  [in] position 撒く中心。
        void Burst(Tsukino::Asset::AssetHandle texture, const BurstParams& params, const hlslpp::float3& position);

    private:
        Tsukino::ECS::Scene&          m_scene;           // 粒子を作るシーン
        Tsukino::Asset::AssetManager& m_assetManager;    // アセット管理
        std::mt19937                  m_rng;             // ばらつき用の乱数
        Tsukino::Asset::AssetHandle   m_soft;            // 柔らかい丸
        Tsukino::Asset::AssetHandle   m_spark;           // 十字の光
        Tsukino::Asset::AssetHandle   m_ink;             // 墨の滴
        Tsukino::Asset::AssetHandle   m_confetti;        // 紙片
    };

}    // namespace ChronoSort::Effects
