//
// Created by andy on 11/16/25.
//

#pragma once

#include <concepts>
#include <functional>
#include <thread>
#include <vector>

namespace neuron {
    template <typename WorkerInfo>
    struct worker_info;

    template <>
    struct worker_info<void> {
        std::size_t index;
    };

    template <typename WorkerInfo>
        requires(!std::same_as<WorkerInfo, void>)
    struct worker_info<WorkerInfo> {
        std::size_t index;
        WorkerInfo  info;
    };
    template <typename WorkerInfo>
    struct X : worker_info<WorkerInfo> {};

    static constexpr std::size_t worker_pool_hardware_concurrency = 0;


    /**
     * Simple thread worker pool.
     *
     * @tparam WorkerCount The number of workers to use (or 0 to use hardware concurrency instead)
     * @tparam WorkerInfo Optional type of extra data to pass to all worker threads (this should be used for more advanced worker setups instead of using plain global state
     * everywhere)
     */
    template <std::size_t WorkerCount = 0, typename WorkerInfo = void>
    class worker_pool {
      public:
        static constexpr std::size_t worker_count = WorkerCount;
        using worker_info_t                       = worker_info<WorkerInfo>;

        explicit worker_pool(const std::function<void(worker_info_t)> &worker_function)
            requires(std::same_as<WorkerInfo, void>)
        {
            if constexpr (worker_count > 0) {
                _workers.reserve(worker_count);
            } else {
                _workers.reserve(std::thread::hardware_concurrency());
            }

            for (std::size_t i = 0; i < worker_count; ++i) {
                _workers.emplace_back(worker_function, worker_info_t{.index = i});
            }
        }

        explicit worker_pool(const std::function<void(worker_info_t)> &worker_function, const WorkerInfo &worker_info)
            requires(!std::same_as<WorkerInfo, void> && std::copyable<WorkerInfo>)
        {
            _workers.reserve(worker_count);
            for (std::size_t i = 0; i < worker_count; ++i) {
                _workers.emplace_back(worker_function, worker_info_t{.index = i, .info = worker_info});
            }
        }

        ~worker_pool() {
            for (auto &worker : _workers) {
                worker.request_stop();
            }
        }

      private:
        std::vector<std::jthread> _workers;
    };

} // namespace neuron::core
