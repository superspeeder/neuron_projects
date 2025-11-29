//
// Created by andy on 11/27/25.
//

#pragma once

#include <bits/this_thread_sleep.h>
#include <chrono>
#include <thread>

namespace neuron {
    class clock {
      public:
        using clock_t      = std::chrono::high_resolution_clock;
        using duration_t   = std::chrono::duration<double>;
        using time_point_t = std::chrono::time_point<clock_t, duration_t>;

        inline clock(const duration_t initial_delta = duration_t(1.0 / 60.0)) : _this_tick(clock_t::now()), _delta(initial_delta) { _last_tick = _this_tick - _delta; }

        [[nodiscard]] time_point_t last_tick() const { return _last_tick; }
        [[nodiscard]] time_point_t this_tick() const { return _this_tick; }
        [[nodiscard]] duration_t   delta() const { return _delta; }

        inline void tick() {
            _last_tick = _this_tick;
            _this_tick = clock_t::now();
            _delta     = _this_tick - _last_tick;
        }

        inline void tick_fps(const double fps) {
            const duration_t min_duration(1.0 / fps);
            tick(min_duration);
        }

        inline void tick(const duration_t min_duration) {
            while (clock_t::now() - _this_tick < min_duration) {}

            tick();
        }

        inline void tick_nobusy(const duration_t min_duration) {
            while (clock_t::now() - _this_tick < min_duration) {
                std::this_thread::yield();
            }

            tick();
        }

        [[nodiscard]] inline double fps() const {
            return 1.0 / _delta.count();
        }
      private:
        time_point_t _last_tick;
        time_point_t _this_tick;
        duration_t   _delta;
    };

} // namespace neuron
