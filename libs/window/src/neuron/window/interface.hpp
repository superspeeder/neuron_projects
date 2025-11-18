//
// Created by andy on 11/17/25.
//

#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace neuron::window {
    class window;

    class system : public std::enable_shared_from_this<system> {
      public:
        system()          = default;
        virtual ~system() = default;

        system(const system &)            = delete;
        system(system &&)                 = delete;
        system &operator=(const system &) = delete;
        system &operator=(system &&)      = delete;

        virtual const std::vector<const char *> &required_instance_extensions() = 0;
        virtual void                             poll()                         = 0;

        virtual std::shared_ptr<window> create_window(int width, int height, std::string_view title) = 0;
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

    class window : public std::enable_shared_from_this<window> {
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

        virtual vk::raii::SurfaceKHR create_surface(const vk::raii::Instance& instance) = 0;

    protected:
        std::shared_ptr<system> _system;
    };
} // namespace neuron::window
