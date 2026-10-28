//----------------------------------------------------------------------------
//! @file   Prefabs.hpp
//! @brief  ゲームの Prefab（ChronoSort/Assets/Prefabs）の一覧と生成
//! @detail 卓の上の物（カード・山札の裏・目印・卓）とステージ（カメラ・光・後処理）は、
//!         コードで組み立てずに Prefab の JSON から作ります。値を変えたいときは JSON を直します。
//!         ゲームのコンポーネントは起動時に RegisterComponents でエンジンの PrefabFactory に登録します。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <string>
#include <unordered_map>

namespace Tsukino::Engine::ECS::Prefab {
    class PrefabFactory;
}    // namespace Tsukino::Engine::ECS::Prefab

namespace ChronoSort::Game::Prefabs {

    // 卓の上の物
    constexpr const char* kCard   = "ChronoSort/Assets/Prefabs/Board/Card/Prefab.json";      // カード（表裏の面は生成後に差し替える）
    constexpr const char* kCover  = "ChronoSort/Assets/Prefabs/Board/Cover/Prefab.json";     // 山札の厚みを見せる裏向きのカード
    constexpr const char* kMarker = "ChronoSort/Assets/Prefabs/Board/Marker/Prefab.json";    // 挿入位置の目印
    constexpr const char* kTable  = "ChronoSort/Assets/Prefabs/Board/Table/Prefab.json";     // 卓

    // ステージ（名前付きのエンティティのまとまり。Camera・UICamera などの名前で引く）
    constexpr const char* kGameStage   = "ChronoSort/Assets/Prefabs/Stage/Game.json";      // 対局の卓
    constexpr const char* kDeskStage   = "ChronoSort/Assets/Prefabs/Stage/Desk.json";      // メニュー画面の背景の机
    constexpr const char* kUIOnlyStage = "ChronoSort/Assets/Prefabs/Stage/UIOnly.json";    // 画面 UI だけの画面

    //! ステージを生成した結果（名前 → エンティティ）です。
    using StageInstance = std::unordered_map<std::string, Tsukino::ECS::Entity>;

    //! ゲームのコンポーネントを PrefabFactory に登録します。起動時に1回呼びます。
    //! @param  [in,out] factory エンジンの PrefabFactory。
    void RegisterComponents(Tsukino::Engine::ECS::Prefab::PrefabFactory& factory);

    //! Prefab から1体を生成します。
    //! @param  [in,out] registry 生成先のレジストリ（エンジンのコンテキストを持っていること）。
    //! @param  [in]     path     Prefab の JSON。
    //! @return 生成したエンティティ。失敗したら entt::null。
    Tsukino::ECS::Entity Spawn(Tsukino::ECS::Registry& registry, const char* path);

    //! 名前付きのまとまりを生成します。
    //! @param  [in,out] registry 生成先のレジストリ。
    //! @param  [in]     path     まとまりの JSON。
    //! @return 名前 → 生成したエンティティ。
    StageInstance SpawnGroup(Tsukino::ECS::Registry& registry, const char* path);

}    // namespace ChronoSort::Game::Prefabs
