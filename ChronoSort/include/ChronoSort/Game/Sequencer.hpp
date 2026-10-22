//----------------------------------------------------------------------------
//! @file   Sequencer.hpp
//! @brief  時間差で処理を呼ぶ小さな段取り表
//! @detail 「0.35 秒後に裏返す、0.8 秒後に叩きつける」のような演出の段取りを、
//!         コールバックの予約として書けるようにします。
//----------------------------------------------------------------------------
#pragma once
#include <functional>
#include <vector>

namespace ChronoSort::Game {

    //! 時間差で処理を呼ぶ段取り表です。
    class Sequencer {
    public:

        //! 処理を予約します。
        //! @param  [in] delay    今から何秒後に呼ぶか。
        //! @param  [in] callback 呼ぶ処理。
        void Schedule(float delay, std::function<void()> callback);

        //! 時間を進め、時刻が来た処理を予約順に呼びます。
        //! 呼ばれた処理の中から新たに予約してもかまいません。
        //! @param  [in] deltaTime 経過時間（秒）。
        void Update(float deltaTime);

        //! 予約をすべて取り消します。
        void Clear();

        //! 予約が残っているかを返します。
        //! @return 残っていれば true。
        bool IsBusy() const { return !m_entries.empty(); }

    private:
        //! 予約1件です。
        struct Entry {
            float                 time;        // 呼ぶ時刻
            unsigned              order;       // 予約順（同じ時刻のときの並び）
            std::function<void()> callback;    // 呼ぶ処理
        };

        std::vector<Entry> m_entries;       // 予約
        float              m_now   = 0.0f;  // 現在時刻
        unsigned           m_order = 0;     // 次の予約順
    };

}    // namespace ChronoSort::Game
