//----------------------------------------------------------------------------
//! @file   CardFactory.cpp
//! @brief  卓とカードのエンティティ生成の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Board/CardFactory.hpp>

#include <ChronoSort/ECS/Component/CardMotionComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>

#include <string>

namespace ChronoSort::Board {

    namespace {
        constexpr float kPi = 3.14159265f;

        // assimp は .obj を読むとき、.mtl のマテリアルより前に DefaultMaterial を 0 番に入れる。
        // そのため .mtl に書いた順（Front, Back / Table）は 1 番から始まる
        constexpr size_t kFirstObjMaterialSlot = 1;

        constexpr const char* kCardMeshPath   = "ChronoSort/Assets/Meshes/card.obj";
        constexpr const char* kTableMeshPath  = "ChronoSort/Assets/Meshes/table.obj";
        constexpr const char* kTableMaterial  = "ChronoSort/Assets/Textures/Table/table.tmat";
        constexpr const char* kCardTextureDir = "ChronoSort/Assets/Textures/Cards/";
    }    // namespace

    //----------------------------------------------------------------------------
    //! 年号の面を上に向けた向きを返します。
    //----------------------------------------------------------------------------
    hlslpp::quaternion RevealedRotation() {
        return hlslpp::quaternion::rotation_z(kPi);
    }

    //----------------------------------------------------------------------------
    //! 生成先のシーンとアセット管理を受け取ります。
    //----------------------------------------------------------------------------
    CardFactory::CardFactory(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetManager& assetManager)
        : m_scene(scene)
        , m_assetManager(assetManager) {
    }

    //----------------------------------------------------------------------------
    //! カードを1枚作ります。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity CardFactory::CreateCard(const Rules::CardData& card, const hlslpp::float3& position, const hlslpp::quaternion& rotation) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        Tsukino::ECS::Entity    entity   = m_scene.CreateEntity();

        auto& transform    = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(entity);
        transform.position = position;
        transform.rotation = rotation;

        // カード固有の面を差し替える。AssetManager は同じパスを一度しか読まないので、毎回 Load してよい
        const std::string facePrefix = std::string(kCardTextureDir) + card.id;
        auto&             model      = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(entity);
        model.modelHandle            = m_assetManager.Load(Tsukino::Core::Path(kCardMeshPath));
        model.materials.resize(kFirstObjMaterialSlot + 2);
        model.materials[kFirstObjMaterialSlot]     = m_assetManager.Load(Tsukino::Core::Path(facePrefix + "_front.tmat"));
        model.materials[kFirstObjMaterialSlot + 1] = m_assetManager.Load(Tsukino::Core::Path(facePrefix + "_back.tmat"));

        auto& motion          = registry.AddComponent<ECS::CardMotionComponent>(entity);
        motion.targetPosition = position;
        motion.targetRotation = rotation;
        return entity;
    }

    //----------------------------------------------------------------------------
    //! 卓を作ります。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity CardFactory::CreateTable() {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        Tsukino::ECS::Entity    entity   = m_scene.CreateEntity();

        registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(entity);

        auto& model       = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(entity);
        model.modelHandle = m_assetManager.Load(Tsukino::Core::Path(kTableMeshPath));
        model.materials.resize(kFirstObjMaterialSlot + 1);
        model.materials[kFirstObjMaterialSlot] = m_assetManager.Load(Tsukino::Core::Path(kTableMaterial));
        return entity;
    }

}    // namespace ChronoSort::Board
