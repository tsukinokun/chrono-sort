//----------------------------------------------------------------------------
//! @file   CountUpTextSystem.hpp
//! @brief  数字の文字を数え上げるシステム
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

namespace ChronoSort::ECS {

    //! CountUpTextComponent の値を進め、整数が変わったら FontComponent の文字を書き換えます。
    class CountUpTextSystem : public Tsukino::ECS::ISystem {
    public:

        //! 数え上げを進めます。
        //! @param  [in,out] registry  レジストリ。
        //! @param  [in]     deltaTime 経過時間（秒）。
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace ChronoSort::ECS
