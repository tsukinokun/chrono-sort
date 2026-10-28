//----------------------------------------------------------------------------
//! @file   LifetimeSystem.hpp
//! @brief  時間が来た UI を子ごと片付けるシステム
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

namespace ChronoSort::ECS {

    //! LifetimeComponent の残り時間を減らし、0 になったものを Transform の子孫ごと破棄します。
    class LifetimeSystem : public Tsukino::ECS::ISystem {
    public:

        //! 残り時間を減らして片付けます。
        //! @param  [in,out] registry  レジストリ。
        //! @param  [in]     deltaTime 経過時間（秒）。
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

    //! エンティティと、Transform で子にしているものをまとめて破棄します（フレームの終わりに消える）。
    //! @param  [in,out] registry レジストリ。
    //! @param  [in]     entity   破棄するエンティティ。entt::null なら何もしない。
    void DestroyTree(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity);

}    // namespace ChronoSort::ECS
