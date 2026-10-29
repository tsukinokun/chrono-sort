//----------------------------------------------------------------------------
//! @file   TuningSet.cpp
//! @brief  調整値の JSON をまとめて読み、持っておくものの実装
//----------------------------------------------------------------------------
#include <ChronoSort/Game/TuningSet.hpp>

#include <ChronoSort/Game/GameServices.hpp>
#include <ChronoSort/Util/AssetPaths.hpp>

#include <Tsukino/Core/IO/JsonFile.hpp>

#include <string>

namespace ChronoSort::Game {

    namespace {
        //----------------------------------------------------------------------------
        //! 調整値の JSON を読む関数を作ります。
        //! @param  [in] path 読むファイル。
        //! @return 読む関数（初期値の入った値へ読み、読めたかを返す）。
        //----------------------------------------------------------------------------
        template <class T>
        typename Tsukino::IO::HotReloadable<T>::Loader Loader(const std::string& path) {
            return [path](T& value) { return Tsukino::IO::TryLoadJson(path, "tuning", value); };
        }

    }    // namespace

#define CHRONOSORT_TUNING(member, type, name) member(Util::AssetPaths::Tuning(name), Loader<type>(Util::AssetPaths::Tuning(name)))

    //----------------------------------------------------------------------------
    //! すべての調整値の JSON を読みます。
    //----------------------------------------------------------------------------
    TuningSet::TuningSet()
        : CHRONOSORT_TUNING(m_rules, Rules::RulesTuning, "Rules")
        , CHRONOSORT_TUNING(m_board, Board::BoardTuning, "Board")
        , CHRONOSORT_TUNING(m_reveal, Match::RevealTuning, "Reveal")
        , CHRONOSORT_TUNING(m_input, Match::InputTuning, "Input")
        , CHRONOSORT_TUNING(m_cpu, Match::CpuTuning, "Cpu")
        , CHRONOSORT_TUNING(m_deal, Match::DealTuning, "Deal")
        , CHRONOSORT_TUNING(m_feedback, Match::FeedbackTuning, "Feedback")
        , CHRONOSORT_TUNING(m_stage, Scene::StageTuning, "Stage")
        , CHRONOSORT_TUNING(m_particles, Effects::ParticleTuning, "Particles")
        , CHRONOSORT_TUNING(m_sounds, SoundTuning, "Sounds") {
    }

#undef CHRONOSORT_TUNING

    //----------------------------------------------------------------------------
    //! 今の調整値を返します。
    //----------------------------------------------------------------------------
    const TuningSet& Tuning() {
        return Services().Tunings();
    }

}    // namespace ChronoSort::Game
