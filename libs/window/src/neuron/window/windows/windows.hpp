//
// Created by andy on 11/18/2025.
//

#pragma once
#if defined(NEURON_WINDOW_TARGET_WINDOWS) && defined(NEURON_WINDOW_WIN32_SUPPORT)
#include "neuron/window/interface.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE

#include <Windows.h>

namespace neuron::window {
    class windows_system : public system {
      public:
        windows_system();
        ~windows_system() override;

        const std::vector<const char *> &required_extensions(render_interface::instance_extension) const override;

        void poll() override;

        std::shared_ptr<window> create_window(int width, int height, std::string_view title) override;

        [[nodiscard]] HINSTANCE instance() const noexcept { return _instance; };

        std::variant<bool, std::monostate> get_presentation_support(const vk::raii::Instance& instance, vk::PhysicalDevice physical_device, uint32_t queue_family) const override;

      private:
        HINSTANCE _instance;
    };

    class windows_window : public window {
      public:
        windows_window(const std::shared_ptr<windows_system> &system, int width, int height, std::string_view title);
        ~windows_window() override;

        bool                 should_close() const override;
        window_size_t        size() const override;
        void                 set_title(std::string_view title) override;
        vk::raii::SurfaceKHR create_surface(const vk::raii::Instance &instance) override;

        void request_redraw() override;

        // inline vk::Extent2D current_extent() const override;

      private:
        HWND          _window;
        bool          _should_close = false;
        window_size_t _size;
        window_pos_t  _pos;

        friend class windows_system;

        static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        LRESULT _window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    };
} // namespace neuron::window

#endif
