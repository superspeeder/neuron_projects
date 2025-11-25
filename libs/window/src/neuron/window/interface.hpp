//
// Created by andy on 11/17/25.
//

#pragma once
#include <functional>
#include <memory>
#include <variant>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include <neuron/render_interface/interface.hpp>

namespace neuron::window {
    class window;

    class system : public std::enable_shared_from_this<system>, public render_interface::extension_requirement_provider<render_interface::instance_extension> {
      public:
        system() = default;
        ~system() override;

        system(const system &)            = delete;
        system(system &&)                 = delete;
        system &operator=(const system &) = delete;
        system &operator=(system &&)      = delete;

        virtual void poll() = 0;

        virtual std::shared_ptr<window>            create_window(int width, int height, std::string_view title) = 0;
        virtual std::variant<bool, std::monostate> get_presentation_support(const vk::raii::Instance &instance, vk::PhysicalDevice physical_device, uint32_t queue_family) const {
            return std::monostate{};
        }
    };

    struct window_size_t {
        int width, height;

        inline window_size_t() = default;
        inline window_size_t(const int width, const int height) : width(width), height(height) {}
        inline window_size_t(const vk::Extent2D &extent) : width(extent.width), height(extent.height) {}

        inline operator vk::Extent2D() const { return vk::Extent2D(width, height); }

        friend bool operator==(const window_size_t &lhs, const window_size_t &rhs) { return lhs.width == rhs.width && lhs.height == rhs.height; }
        friend bool operator!=(const window_size_t &lhs, const window_size_t &rhs) { return !(lhs == rhs); }

        friend void swap(window_size_t &lhs, window_size_t &rhs) noexcept {
            using std::swap;
            swap(lhs.width, rhs.width);
            swap(lhs.height, rhs.height);
        }
    };

    extern system *global;


    struct window_pos_t {
        int x, y;

        inline window_pos_t() = default;
        inline window_pos_t(const int x, const int y) : x(x), y(y) {}
        inline window_pos_t(const vk::Offset2D &off) : x(off.x), y(off.y) {}

        inline operator vk::Offset2D() const { return vk::Offset2D(x, y); }

        friend bool operator==(const window_pos_t &lhs, const window_pos_t &rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
        friend bool operator!=(const window_pos_t &lhs, const window_pos_t &rhs) { return !(lhs == rhs); }

        friend void swap(window_pos_t &lhs, window_pos_t &rhs) noexcept {
            using std::swap;
            swap(lhs.x, rhs.x);
            swap(lhs.y, rhs.y);
        }
    };

    class window : public std::enable_shared_from_this<window>, public render_interface::surface_provider {
      public:
        explicit window(std::shared_ptr<system> system) : _system(system) {}
        virtual ~window() = default;

        window(const window &other)                = delete;
        window(window &&other) noexcept            = delete;
        window &operator=(const window &other)     = delete;
        window &operator=(window &&other) noexcept = delete;

        virtual bool should_close() const = 0;

        virtual window_size_t size() const = 0;

        virtual void set_title(std::string_view title) = 0;

        inline std::function<void(bool &close)> set_on_close_callback(const std::function<void(bool &close)> &f) {
            auto old          = _on_close_request;
            _on_close_request = f;
            return old;
        };

        inline std::function<void()> set_on_redraw_callback(const std::function<void()> &f) {
            auto old   = _on_redraw;
            _on_redraw = f;
            return old;
        }

        // not supported by all systems, so we can have this default to call _on_redraw
        virtual void request_redraw() { _on_redraw(); };

        inline vk::Extent2D current_extent() const override { return size(); };

      protected:
        std::shared_ptr<system> _system;

        std::function<void(bool &close)> _on_close_request = [](bool &close) {};
        std::function<void()>            _on_redraw        = [] {};
    };
} // namespace neuron::window
