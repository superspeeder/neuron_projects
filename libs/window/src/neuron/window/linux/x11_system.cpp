//
// Created by andy on 11/17/25.
//
#if defined(NEURON_WINDOW_TARGET_LINUX) && defined(NEURON_WINDOW_X11_SUPPORT)

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan_raii.hpp>

#include "x11_system.hpp"


namespace neuron::window {
    const std::vector<const char *> &x11_system::required_extensions(render_interface::instance_extension) const {
        static std::vector<const char *> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
        };
        return extensions;
    }

    void x11_system::poll() {
        XEvent event{};
        while (XPending(_display) > 0) {
            XNextEvent(_display, &event);
            _dispatch_event(event);
        }
    }

    void x11_system::_dispatch_event(XEvent &event) {
    }
} // namespace neuron::window

#endif