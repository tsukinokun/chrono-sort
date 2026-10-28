//----------------------------------------------------------------------------
//! @file   CardMotionComponentSerialization.hpp
//! @brief  CardMotionComponent の Prefab 用のシリアライズ
//! @detail Prefab に書くのは追従の速さだけです。目標・現在の姿勢と演出の状態は実行時に決まるので書きません。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/ECS/Component/CardMotionComponent.hpp>

#include <cereal/cereal.hpp>

namespace ChronoSort::ECS {

    //----------------------------------------------------------------------------
    //! 追従の速さを書き出します。
    //! @param  [in,out] archive アーカイブ。
    //! @param  [in]     motion  コンポーネント。
    //----------------------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const CardMotionComponent& motion) {
        archive(cereal::make_nvp("positionSharpness", motion.positionSharpness), cereal::make_nvp("rotationSharpness", motion.rotationSharpness));
    }

    //----------------------------------------------------------------------------
    //! 追従の速さを読み込みます。書かれていない項目は既定値のままにします。
    //! @param  [in,out] archive アーカイブ。
    //! @param  [out]    motion  コンポーネント。
    //----------------------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, CardMotionComponent& motion) {
        try {
            archive(cereal::make_nvp("positionSharpness", motion.positionSharpness));
        } catch(const cereal::Exception&) {
        }
        try {
            archive(cereal::make_nvp("rotationSharpness", motion.rotationSharpness));
        } catch(const cereal::Exception&) {
        }
    }

}    // namespace ChronoSort::ECS
