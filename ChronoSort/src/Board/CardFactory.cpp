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

        constexpr const char* kCoverMaterial         = "ChronoSort/Assets/Textures/Board/cover.tmat";
        constexpr const char* kMarkerMaterial        = "ChronoSort/Assets/Textures/Board/marker.tmat";
        constexpr const char* kMarkerWarningMaterial = "ChronoSort/Assets/Textures/Board/marker_warning.tmat";
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

        // 生成した直後に Fly を呼べるよう、現在の姿勢もここで決めておく
        auto& motion          = registry.AddComponent<ECS::CardMotionComponent>(entity);
        motion.targetPosition = position;
        motion.targetRotation = rotation;
        motion.position       = position;
        motion.rotation       = rotation;
        motion.initialized    = true;
        return entity;
    }

    //----------------------------------------------------------------------------
    //! 山札の厚みを見せる、両面とも裏模様のカードを作ります。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity CardFactory::CreateCover(const hlslpp::float3& position, const hlslpp::quaternion& rotation) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        Tsukino::ECS::Entity    entity   = m_scene.CreateEntity();

        auto& transform    = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(entity);
        transform.position = position;
        transform.rotation = rotation;

        const Tsukino::Asset::AssetHandle cover = m_assetManager.Load(Tsukino::Core::Path(kCoverMaterial));
        auto&                             model = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(entity);
        model.modelHandle                       = m_assetManager.Load(Tsukino::Core::Path(kCardMeshPath));
        model.materials.resize(kFirstObjMaterialSlot + 2);
        model.materials[kFirstObjMaterialSlot]     = cover;
        model.materials[kFirstObjMaterialSlot + 1] = cover;
        return entity;
    }

    //----------------------------------------------------------------------------
    //! 場の列の挿入位置を示す、枠だけのカードを作ります。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Entity CardFactory::CreateMarker() {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        Tsukino::ECS::Entity    entity   = m_scene.CreateEntity();
        registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(entity);

        auto& model       = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(entity);
        model.modelHandle = m_assetManager.Load(Tsukino::Core::Path(kCardMeshPath));
        model.visible     = false;
        model.materials.resize(kFirstObjMaterialSlot + 2);
        SetMarkerWarning(entity, false);

        auto& motion = registry.AddComponent<ECS::CardMotionComponent>(entity);
        motion.positionSharpness = 18.0f;
        return entity;
    }

    //----------------------------------------------------------------------------
    //! 赤い（警告の）目印用のマテリアルに切り替えます。
    //----------------------------------------------------------------------------
    void CardFactory::SetMarkerWarning(Tsukino::ECS::Entity marker, bool warning) {
        const Tsukino::Asset::AssetHandle material = m_assetManager.Load(Tsukino::Core::Path(warning ? kMarkerWarningMaterial : kMarkerMaterial));
        auto&                             model    = m_scene.GetRegistry().GetComponent<Tsukino::BuiltIn::ECS::ModelComponent>(marker);
        model.materials[kFirstObjMaterialSlot]     = material;
        model.materials[kFirstObjMaterialSlot + 1] = material;
    }

    //----------------------------------------------------------------------------
    //! エンティティを破棄します。
    //----------------------------------------------------------------------------
    void CardFactory::Destroy(Tsukino::ECS::Entity entity) {
        m_scene.GetRegistry().QueueDestroy(entity);
    }

    //----------------------------------------------------------------------------
    //! 生成先のシーンのレジストリを返します。
    //----------------------------------------------------------------------------
    Tsukino::ECS::Registry& CardFactory::Registry() {
        return m_scene.GetRegistry();
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
