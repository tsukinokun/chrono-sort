//----------------------------------------------------------------------------
//! @file   CardFactory.hpp
//! @brief  卓とカードのエンティティ生成
//! @detail 形は Prefab（ChronoSort/Assets/Prefabs/Board）から作り、ここではカードごとに違う所だけを決めます。
//!         カードは表裏で別マテリアルを持つ薄い板（card.obj）です。
//!         面の画像と .tmat は Tools/art/bake_cards.py で焼き込んだものを、カード ID から引きます。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Rules/CardData.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>

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

        //! 山札の厚みを見せる、両面とも裏模様のカードを作ります。
        //! @param  [in] position 置く位置。
        //! @param  [in] rotation 置く向き。
        //! @return 作成したエンティティ。
        Tsukino::ECS::Entity CreateCover(const hlslpp::float3& position, const hlslpp::quaternion& rotation);

        //! 場の列の挿入位置を示す、枠だけのカードを作ります。
        //! @return 作成したエンティティ。
        Tsukino::ECS::Entity CreateMarker();

        //! 赤い（警告の）目印用のマテリアルに切り替えます。
        //! @param  [in] marker  目印のエンティティ。
        //! @param  [in] warning 赤にするなら true。
        void SetMarkerWarning(Tsukino::ECS::Entity marker, bool warning);

        //! エンティティを破棄します。
        //! @param  [in] entity 破棄するエンティティ。
        void Destroy(Tsukino::ECS::Entity entity);

        //! 生成先のシーンのレジストリを返します。
        //! @return レジストリ。
        Tsukino::ECS::Registry& Registry();

    private:

        //! 生成したエンティティを、指定した姿勢に置きます。
        //! @param  [in] entity   エンティティ。
        //! @param  [in] position 位置。
        //! @param  [in] rotation 向き。
        void PlaceAt(Tsukino::ECS::Entity entity, const hlslpp::float3& position, const hlslpp::quaternion& rotation);

        Tsukino::ECS::Scene&          m_scene;           // 生成先のシーン
        Tsukino::Asset::AssetManager& m_assetManager;    // アセット管理
    };

}    // namespace ChronoSort::Board
