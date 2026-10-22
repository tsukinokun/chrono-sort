//----------------------------------------------------------------------------
//! @file   DeskStage.hpp
//! @brief  メニュー画面の背景（カードが散らばった研究者の机）
//! @detail タイトルやクレジットの背景として、3D の卓にカードを無造作に散らし、
//!         カメラをゆっくり漂わせます。システムの登録もまとめて行います。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Board/CardFactory.hpp>

#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <memory>
#include <random>

namespace Tsukino::ECS {
    class Scene;
}    // namespace Tsukino::ECS

namespace ChronoSort::Scene {

    //! メニュー画面の背景です。
    class DeskStage {
    public:

        //! 背景を作ります（システムの登録・カメラ・ライト・卓・散らばったカード）。
        //! @param  [in] scene     作り先のシーン。
        //! @param  [in] cardCount 散らすカードの枚数。
        void Build(Tsukino::ECS::Scene& scene, int cardCount);

        //! カメラを漂わせます。
        //! @param  [in] deltaTime 経過時間（秒）。
        void Update(float deltaTime);

    private:
        Tsukino::ECS::Scene*                m_scene  = nullptr;       // 作り先のシーン
        std::unique_ptr<Board::CardFactory> m_factory;                // カードの生成
        Tsukino::ECS::Entity                m_camera = entt::null;    // カメラ
        float                               m_time   = 0.0f;          // 経過時間
        std::mt19937                        m_rng;                    // 散らし方の乱数
    };

}    // namespace ChronoSort::Scene
