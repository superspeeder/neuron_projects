//
// Created by andy on 11/24/2025.
//

#pragma once

#include <functional>
#include <typeindex>
#include <vector>

namespace neuron::events {

    namespace detail {

        class function_list;

        template <typename T>
        struct t_function_handle {
          private:
            void (*_handle)(void *);
            friend class function_list;

            constexpr t_function_handle(void (*handle)(T *)) : _handle(handle) {}

            template <typename T2>
            constexpr explicit operator t_function_handle<T2>() const {
                return reinterpret_cast<void (*)(T2 *)>(_handle);
            }
        };

        using function_handle = t_function_handle<void>;

        /**
         * Important Design Decision: This function list only supports ordinary functions (no capturing lambdas or pointer-to-member functions). This is to avoid extra indirection
         * and allow for easier pointer casting.
         *
         */
        class function_list {
          public:
            function_list()                                          = default;
            function_list(const function_list &other)                = delete;
            function_list(function_list &&other) noexcept            = default;
            function_list &operator=(const function_list &other)     = delete;
            function_list &operator=(function_list &&other) noexcept = default;

            inline function_handle add_function(void (*function)(void *)) {
                _list.push_back(function);
                return function;
            };

            /**
             * @brief Remove a function from this list
             * @param function The handle of the added function (returned by add_function)
             */
            inline void remove_function(const function_handle function) {
                if (const auto it = std::ranges::find(_list, function._handle); it != _list.end()) {
                    _list.erase(it);
                }
            }

            inline void clear() { _list.clear(); }

            inline std::size_t size() const { return _list.size(); }

            inline void invoke(void *data) const {
                for (const auto function : _list)
                    function(data);
            }

          private:
            std::vector<void (*)(void *)> _list;
        };

        template <typename T>
        class typed_function_list {
          public:
            constexpr explicit typed_function_list(function_list &list) : _list(list) {}

            // ReSharper disable CppMemberFunctionMayBeConst
            inline t_function_handle<T> add_function(void (*function)(T *)) {
                return static_cast<t_function_handle<T>>(_list.add_function(reinterpret_cast<void (*)(void *)>(function)));
            }
            inline void remove_function(const t_function_handle<T> function) { _list.remove_function(static_cast<function_handle>(function)); }
            inline void clear() { _list.clear(); }
            // ReSharper restore CppMemberFunctionMayBeConst

            inline std::size_t size() const { return _list.size(); }
            inline void        invoke(T *data) const {
                if constexpr (std::is_const_v<T>) {
                    _list.invoke(static_cast<void *>(const_cast<T *>(data)));
                } else {
                    _list.invoke(static_cast<void *>(data));
                }
            }

          private:
            function_list &_list;
        };
    } // namespace detail

    using function_handle = detail::function_handle;

    template <typename T>
    using typed_function_handle = detail::t_function_handle<T>;

    template <typename E>
    class event_bus {
      public:
        event_bus() = default;

        event_bus(const event_bus &other)                = delete;
        event_bus(event_bus &&other) noexcept            = delete;
        event_bus &operator=(const event_bus &other)     = delete;
        event_bus &operator=(event_bus &&other) noexcept = delete;

        void dispatch(E type, void *data) {
            if (auto it = _listeners.find(type); it != _listeners.end()) {
                it->second.invoke(data);
            }
        }

        function_handle add_listener(E type, void (*listener)(void *)) {
            if (auto it = _listeners.find(type); it != _listeners.end()) {
                return it->second.add_function(listener);
            }

            auto [it, _] = _listeners.insert_or_assign(type, detail::function_list());
            return it->second.add_function(listener);
        }

        void remove_listener(E type, function_handle handle) {
            if (auto it = _listeners.find(type); it != _listeners.end()) {
                it->second.remove_function(handle);
            }
        }

        template <typename D>
        void dispatch(D *event)
            requires(std::same_as<E, std::type_index>)
        {
            dispatch(std::type_index(typeid(D)), static_cast<void *>(event));
        }

        template <typename D>
        typed_function_handle<D> add_listener(void (*listener)(D *))
            requires(std::same_as<E, std::type_index>)
        {
            return static_cast<typed_function_handle<D>>(add_listener(std::type_index(typeid(D)), reinterpret_cast<void (*)(void *)>(listener)));
        }

        template <typename D>
        void remove_listener(typed_function_handle<D> handle)
            requires(std::same_as<E, std::type_index>)
        {
            remove_listener(std::type_index(typeid(D)), static_cast<function_handle>(handle));
        }
      private:
        std::unordered_map<E, detail::function_list> _listeners;
    };

    using typed_event_bus = event_bus<std::type_index>;

    extern event_bus<std::type_index> global_bus;
} // namespace neuron::events
