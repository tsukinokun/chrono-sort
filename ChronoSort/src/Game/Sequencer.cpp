//----------------------------------------------------------------------------
//! @file   Sequencer.cpp
//! @brief  時間差で処理を呼ぶ段取り表の実装
//----------------------------------------------------------------------------
#include <ChronoSort/Game/Sequencer.hpp>

#include <algorithm>

namespace ChronoSort::Game {

    //----------------------------------------------------------------------------
    //! 処理を予約します。
    //----------------------------------------------------------------------------
    void Sequencer::Schedule(float delay, std::function<void()> callback) {
        m_entries.push_back({m_now + delay, m_order++, std::move(callback)});
    }

    //----------------------------------------------------------------------------
    //! 時間を進め、時刻が来た処理を予約順に呼びます。
    //----------------------------------------------------------------------------
    void Sequencer::Update(float deltaTime) {
        m_now += deltaTime;
        for(;;) {
            // 呼んだ処理の中で予約が増えることがあるため、1件ずつ取り出して呼ぶ
            auto due = std::min_element(m_entries.begin(), m_entries.end(), [](const Entry& a, const Entry& b) {
                return a.time != b.time ? a.time < b.time : a.order < b.order;
            });
            if(due == m_entries.end() || due->time > m_now) {
                return;
            }
            std::function<void()> callback = std::move(due->callback);
            m_entries.erase(due);
            callback();
        }
    }

    //----------------------------------------------------------------------------
    //! 予約をすべて取り消します。
    //----------------------------------------------------------------------------
    void Sequencer::Clear() {
        m_entries.clear();
    }

}    // namespace ChronoSort::Game
