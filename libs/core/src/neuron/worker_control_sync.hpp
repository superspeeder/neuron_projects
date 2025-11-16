//
// Created by andy on 11/16/25.
//

#pragma once
#include <atomic>
#include <shared_mutex>

namespace neuron::core {
    class worker_control_sync {
    public:
        worker_control_sync() = default;
        ~worker_control_sync() = default;

        worker_control_sync(const worker_control_sync&) = delete;
        worker_control_sync& operator=(const worker_control_sync&) = delete;
        worker_control_sync(worker_control_sync&&) = delete;
        worker_control_sync& operator=(worker_control_sync&&) = delete;

        /**
         * Signal the controller that work needs to be done
         */
        void signal_wait_control();

        /**
         * Signal the workers that the control thread is finished with its work (also release exclusive control over the system)
         */
        void clear_control();

        /**
         * Check if control is waiting, then release ownership, wait for control to be finished, then regrab ownership.
         */
        void check_wait_control();

        /**
         * Wait for workers to signal that control has work to do, then wait for workers to release control over the shared lock.
         */
        void wait_control_signal();

    private:
        std::shared_mutex _activity_owner;
        std::atomic_flag _activity_indicator;
    };

    /// Sample usage for worker_control_sync
    ///
    /// ```cpp
    /// worker_control_sync* sync = ...;
    ///
    /// auto controller = [sync,...](...) {
    ///     while (true) {
    ///         sync->wait_control_signal();
    ///         // do stuff
    ///         sync->clear_control();
    ///     }
    /// }
    ///
    /// auto worker = [sync, ...](...) {
    ///     while (true) {
    ///         sync->check_wait_control();
    ///         auto work = ...;
    ///         // do work
    ///         if (needs_controller_action) {
    ///             sync->signal_wait_control();
    ///             // do work
    ///         }
    ///         // finish work
    ///     }
    /// }
    /// ```
}