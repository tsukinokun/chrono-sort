//----------------------------------------------------------------------------
//! @file   TuningTypes.hpp
//! @brief  調整値の JSON でくり返し使う小さな値の組
//! @detail 追従の速さ・放物線の飛び方・弾み・震え・光り方など、いくつもの調整値で同じ形をとるものです。
//!         どれも書いていない項目は既定値のまま読みます（Tsukino::IO::LoadOptional）。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/IO/CerealOptional.hpp>
#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <hlsl++.h>

#include <cereal/cereal.hpp>

namespace ChronoSort::Game {

    //! 位置と回転の追従の速さです（CardMotion::SetSharpness）。
    struct Sharpness {
        float position = 14.0f;    // 位置の追従の速さ
        float rotation = 10.0f;    // 回転の追従の速さ
    };

    //! 放物線の飛び方です（CardMotion::Fly）。
    struct Flight {
        float seconds = 0.4f;     // 所要時間（秒）
        float arc     = 60.0f;    // 弧の高さ
        float delay   = 0.0f;     // 飛び始めるまでの待ち（秒）
    };

    //! 一瞬大きくなって戻る弾みです（CardMotion::Punch）。
    struct Bump {
        float seconds = 0.3f;     // 長さ（秒）
        float amount  = 0.07f;    // 拡大率に足す最大の量
    };

    //! 左右の震えです（CardMotion::Shake）。
    struct Tremor {
        float seconds   = 0.5f;    // 長さ（秒）
        float amplitude = 7.0f;    // 振れ幅
    };

    //! 光り方です（Glow は減衰、PulseGlow は脈打ち）。
    struct GlowSpec {
        hlslpp::float3 color     = hlslpp::float3(1.0f, 0.8f, 0.3f);    // 発光色（HDR）
        float          intensity = 1.0f;                                 // 強さ
        float          rate      = 1.0f;                                 // Glow なら1秒あたりの減衰、PulseGlow なら脈打ちの速さ
    };

    //! Sharpness を読み込みます。
    template <class Archive>
    void load(Archive& archive, Sharpness& value) {
        Tsukino::IO::LoadOptional(archive, "position", value.position);
        Tsukino::IO::LoadOptional(archive, "rotation", value.rotation);
    }

    //! Flight を読み込みます。
    template <class Archive>
    void load(Archive& archive, Flight& value) {
        Tsukino::IO::LoadOptional(archive, "seconds", value.seconds);
        Tsukino::IO::LoadOptional(archive, "arc", value.arc);
        Tsukino::IO::LoadOptional(archive, "delay", value.delay);
    }

    //! Bump を読み込みます。
    template <class Archive>
    void load(Archive& archive, Bump& value) {
        Tsukino::IO::LoadOptional(archive, "seconds", value.seconds);
        Tsukino::IO::LoadOptional(archive, "amount", value.amount);
    }

    //! Tremor を読み込みます。
    template <class Archive>
    void load(Archive& archive, Tremor& value) {
        Tsukino::IO::LoadOptional(archive, "seconds", value.seconds);
        Tsukino::IO::LoadOptional(archive, "amplitude", value.amplitude);
    }

    //! GlowSpec を読み込みます。
    template <class Archive>
    void load(Archive& archive, GlowSpec& value) {
        Tsukino::IO::LoadOptional(archive, "color", value.color);
        Tsukino::IO::LoadOptional(archive, "intensity", value.intensity);
        Tsukino::IO::LoadOptional(archive, "rate", value.rate);
    }

}    // namespace ChronoSort::Game
