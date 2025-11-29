//
// Created by andy on 11/17/25.
//
#if defined(NEURON_WINDOW_TARGET_LINUX) && defined(NEURON_WINDOW_X11_SUPPORT)

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan_raii.hpp>

#include <vulkan/vulkan_xlib.h>

#include "x11_system.hpp"


namespace neuron::window {
    x11_system::x11_system() {
        _display        = XOpenDisplay(nullptr);
        _default_screen = DefaultScreen(_display);
        _root           = RootWindow(_display, _default_screen);
    }

    x11_system::~x11_system() {}


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

    void x11_system::_dispatch_event(XEvent &event) {}

    std::shared_ptr<window> x11_system::create_window(const int width, const int height, const std::string_view title) {}

    std::variant<bool, std::monostate> x11_system::get_presentation_support(const vk::raii::Instance &instance, const vk::PhysicalDevice physical_device,
                                                                            const uint32_t queue_family) const {
        PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR getSupport = (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR)(instance.getProcAddr(
            "vkGetPhysicalDeviceXlibPresentationSupportKHR"));
        return getSupport(physical_device, queue_family, _display, XVisualIDFromVisual(DefaultVisual(_display, _default_screen))) == vk::True;
    }

} // namespace neuron::window

#endif
