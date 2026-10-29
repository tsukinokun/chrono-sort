//----------------------------------------------------------------------------
//! @file   TuningSet.hpp
//! @brief  調整値の JSON をまとめて読み、持っておくもの
//! @detail ChronoSort/Assets/Data/Tuning/ の JSON を起動時に1回読みます。Debug ビルドでは、保存し直すと
//!         エンジンの HotReload がその場で読み直します（次のフレームから効く。壊れた JSON なら前の値のまま）。
//!         ファイルが無い・項目が無いときは、各構造体の初期値（JSON を書く前の値）を使います。
//!         GameServices が持ち、どこからでも Game::Tuning() で受け取れます。
//----------------------------------------------------------------------------
#pragma once
#include <ChronoSort/Board/BoardTuning.hpp>
#include <ChronoSort/Effects/ParticleTuning.hpp>
#include <ChronoSort/Game/SoundTuning.hpp>
#include <ChronoSort/Match/MatchTuning.hpp>
#include <ChronoSort/Rules/RulesTuning.hpp>
#include <ChronoSort/Scene/StageTuning.hpp>
#include <ChronoSort/UI/PaperInsetTuning.hpp>

#include <Tsukino/Core/IO/HotReloadable.hpp>

namespace ChronoSort::Game {

    //! 調整値の JSON をまとめて読み、持っておくものです。
    //! @note   受け取った参照は持ち続けてよい（読み直しは同じオブジェクトへの代入）。
    //!         ただし中の vector や map の要素への参照は、フレームをまたいで持たないこと
    class TuningSet {
    public:

        //! すべての調整値の JSON を読みます。
        TuningSet();

        //! @return ルール（Rules.json）。
        const Rules::RulesTuning& Rules() const { return m_rules.Get(); }

        //! @return 卓の上の並べ方（Board.json）。
        const Board::BoardTuning& Board() const { return m_board.Get(); }

        //! @return 公開の演出（Reveal.json）。
        const Match::RevealTuning& Reveal() const { return m_reveal.Get(); }

        //! @return 人間の入力（Input.json）。
        const Match::InputTuning& Input() const { return m_input.Get(); }

        //! @return CPU の迷い方（Cpu.json）。
        const Match::CpuTuning& Cpu() const { return m_cpu.Get(); }

        //! @return 配り方と段取りの間（Deal.json）。
        const Match::DealTuning& Deal() const { return m_deal.Get(); }

        //! @return 出来事への返し方（Feedback.json）。
        const Match::FeedbackTuning& Feedback() const { return m_feedback.Get(); }

        //! @return カメラと背景の机（Stage.json）。
        const Scene::StageTuning& Stage() const { return m_stage.Get(); }

        //! @return 粒子の撒き方（Particles.json）。
        const Effects::ParticleTuning& Particles() const { return m_particles.Get(); }

        //! @return 音の合図（Sounds.json）。
        const SoundTuning& Sounds() const { return m_sounds.Get(); }

        //! @return 紙片の画像の縁の欠け（PaperInsets.json）。
        const UI::PaperInsetTuning& PaperInsets() const { return m_paperInsets.Get(); }

    private:
        Tsukino::IO::HotReloadable<Rules::RulesTuning>      m_rules;
        Tsukino::IO::HotReloadable<Board::BoardTuning>      m_board;
        Tsukino::IO::HotReloadable<Match::RevealTuning>     m_reveal;
        Tsukino::IO::HotReloadable<Match::InputTuning>      m_input;
        Tsukino::IO::HotReloadable<Match::CpuTuning>        m_cpu;
        Tsukino::IO::HotReloadable<Match::DealTuning>       m_deal;
        Tsukino::IO::HotReloadable<Match::FeedbackTuning>   m_feedback;
        Tsukino::IO::HotReloadable<Scene::StageTuning>      m_stage;
        Tsukino::IO::HotReloadable<Effects::ParticleTuning> m_particles;
        Tsukino::IO::HotReloadable<SoundTuning>             m_sounds;
        Tsukino::IO::HotReloadable<UI::PaperInsetTuning>    m_paperInsets;
    };

    //! 今の調整値を返します（GameServices を作ってから壊すまでの間だけ呼べる）。
    //! @return 調整値。
    const TuningSet& Tuning();

}    // namespace ChronoSort::Game
