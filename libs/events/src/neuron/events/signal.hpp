//
// Created by andy on 11/25/25.
//

#pragma once
#include <algorithm>
#include <ranges>
#include <vector>

namespace neuron::events {

    template <typename... Args>
    class signal {
      public:
        signal() = default;

        signal(const signal &other)                = delete;
        signal(signal &&other) noexcept            = delete;
        signal &operator=(const signal &other)     = delete;
        signal &operator=(signal &&other) noexcept = delete;

        struct handle {
            friend bool operator==(const handle &lhs, const handle &rhs) { return lhs._handle == rhs._handle && lhs._userdata == rhs._userdata; }
            friend bool operator!=(const handle &lhs, const handle &rhs) { return !(lhs == rhs); }

            handle(const handle &other)                = default;
            handle(handle &&other) noexcept            = default;
            handle &operator=(const handle &other)     = default;
            handle &operator=(handle &&other) noexcept = default;

          private:
            handle(void (*handle)(void *, Args...), void *userdata) : _handle(handle), _userdata(userdata) {}

            void (*_handle)(void *, Args...);
            void *_userdata;

            void operator()(Args &&...args) const { _handle(_userdata, std::forward<Args>(args)...); }

            friend class signal;
        };

        handle add_listener(void (*listener)(void *, Args... args), void* userdata) {
            handle fh = {listener, userdata};
            _callbacks.push_back(fh);
            return fh;
        }

        void remove_listener(handle fh) { std::ranges::remove_if(_callbacks, fh); }

        void dispatch(Args &&...args) const {
            for (const auto &f : _callbacks) {
                f(std::forward<Args>(args)...);
            }
        }

        void operator()(Args &&...args) const {
            for (const auto &f : _callbacks) {
                f(std::forward<Args>(args)...);
            }
        }

        auto& operator+=(std::pair<void (*)(void *, Args... args), void*> p) {
            add_listener(p.first, p.second);
            return *this;
        }

        auto& operator-=(const handle fh) {
            remove_listener(fh);
            return *this;
        }


      private:
        std::vector<handle> _callbacks;
    };

} // namespace neuron::events
