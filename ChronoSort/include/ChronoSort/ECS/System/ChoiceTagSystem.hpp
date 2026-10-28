//----------------------------------------------------------------------------
//! @file   ChoiceTagSystem.hpp
//! @brief  選択肢の札の見た目（文字の色・弾み・持ち上がり・選んだ音）を、選ばれ方に合わせるシステム
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

namespace ChronoSort::ECS {

    //! 選択肢の札の見た目を、UIToggleComponent の入・切に合わせます。
    //! @note   クリックで選び直されたら（changed で入になったら）札を弾ませ、"ui.choose" の音を鳴らす。
    //!         エンジンの UIToggleSystem の後に置くこと
    class ChoiceTagSystem : public Tsukino::ECS::ISystem {
    public:

        //! 札の見た目を更新します。
        //! @param  [in,out] registry  レジストリ。
        //! @param  [in]     deltaTime 経過時間（秒）。
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace ChronoSort::ECS
