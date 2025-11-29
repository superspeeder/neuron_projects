//
// Created by andy on 11/17/25.
//

#pragma once
#if defined(NEURON_WINDOW_TARGET_LINUX) && defined(NEURON_WINDOW_X11_SUPPORT)
#include "./linux.hpp"

#include <X11/Xlib.h>

namespace neuron::window {
    class x11_window;
    class x11_system : public linux_system {
      public:
        x11_system();
        ~x11_system() override;

        const std::vector<const char *> &required_extensions(render_interface::instance_extension) const override;
        void                             poll() override;

        std::shared_ptr<window> create_window(int width, int height, std::string_view title) override;
        std::variant<bool, std::monostate> get_presentation_support(const vk::raii::Instance &instance, vk::PhysicalDevice physical_device, uint32_t queue_family) const override;

      private:
        Display *_display;
        int      _default_screen;
        Window   _root;

        void _dispatch_event(XEvent &event);
        friend class x11_window;
    };

    class x11_window : public window {
    public:
        x11_window(const std::shared_ptr<x11_system>& system, int width, int height, std::string_view title);
        ~x11_window() override;

        
        bool should_close() const;
        window_size_t size() const;
        void set_title(std::string_view title);

        vk::raii::SurfaceKHR create_surface(const vk::raii::Instance& instance) override;

    private:
        Window _window;
        bool _close_requested = false;
    };
} // namespace neuron::window
#endif
