//----------------------------------------------------------------------------
//! @file   CameraRigSystem.hpp
//! @brief  卓のカメラに揺れと漂いを足すシステム
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

namespace ChronoSort::ECS {

    //! 卓のカメラに、揺れと漂いを足します（CameraRigComponent）。一時停止中も漂いは止めません。
    class CameraRigSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

    //! カメラを揺らします（設定で揺れを切っていれば何もしない）。
    //! @param  [in,out] registry レジストリ。
    //! @param  [in]     strength 揺れの強さ。
    void ShakeCamera(Tsukino::ECS::Registry& registry, float strength);

}    // namespace ChronoSort::ECS
