//----------------------------------------------------------------------------
//! @file   CardFactory.hpp
//! @brief  卓とカードのエンティティ生成
//! @detail カードは表裏で別マテリアルを持つ薄い板（card.obj）です。
//!         面の画像と .tmat は Tools/art/bake_cards.py で焼き込んだものを、カード ID から引きます。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/CardData.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <hlsl++.h>

namespace Tsukino::ECS {
    class Scene;
}    // namespace Tsukino::ECS

namespace Tsukino::Asset {
    class AssetManager;
}    // namespace Tsukino::Asset

namespace ChronoSort::Board {

    constexpr float kCardWidth  = 64.0f;    // カードの幅（card.obj と同じ値）
    constexpr float kCardHeight = 96.0f;    // カードの高さ（card.obj と同じ値）

    //! 年号の面を上に向けた向き（表を上にした状態から Z 軸まわりに 180°）を返します。
    //! @return 裏返した向き。
    hlslpp::quaternion RevealedRotation();

    //! 卓とカードのエンティティを作ります。
    class CardFactory {
    public:

        //! 生成先のシーンとアセット管理を受け取ります。
        //! @param  [in] scene        エンティティを作るシーン。
        //! @param  [in] assetManager アセット管理。
        CardFactory(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager);

        //! カードを1枚作ります。
        //! @param  [in] card     カードのデータ。
        //! @param  [in] position 置く位置。
        //! @param  [in] rotation 置く向き。
        //! @return 作成したエンティティ。
        Tsukino::ECS::Entity CreateCard(const Rules::CardData& card, const hlslpp::float3& position, const hlslpp::quaternion& rotation);

        //! 卓を作ります。
        //! @return 作成したエンティティ。
        Tsukino::ECS::Entity CreateTable();

    private:
        Tsukino::ECS::Scene&          m_scene;           // 生成先のシーン
        Tsukino::Asset::AssetManager& m_assetManager;    // アセット管理
    };

}    // namespace ChronoSort::Board
