//
// Created by andy on 11/17/25.
//
#include <cstdio>
#if defined(NEURON_WINDOW_TARGET_LINUX) && defined(NEURON_WINDOW_X11_SUPPORT)

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan_raii.hpp>

#include <vulkan/vulkan_xlib.h>

#include "x11_system.hpp"


namespace neuron::window {
    x11_system::x11_system() {
        _display        = XOpenDisplay(":0");
        _default_screen = DefaultScreen(_display);
        _root           = RootWindow(_display, _default_screen);
    }

    x11_system::~x11_system() {
        XCloseDisplay(_display);
        printf("close");
    }


    const std::vector<const char *> &x11_system::required_extensions(render_interface::instance_extension) const {
        static std::vector<const char *> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
        };
        return extensions;
    }


    void x11_system::poll() {
        Screen* s = DefaultScreenOfDisplay(_display);
        printf("screen res: %dx%d\n", WidthOfScreen(s), HeightOfScreen(s));
        printf("display: %p\n", _display);

        printf("enter poll");
        XEvent event;
        XPending(_display);
        printf("before loop");
        while (QLength(_display)) {
            XNextEvent(_display, &event);
            printf("read event");
            _dispatch_event(event);
        }
    }

    void x11_system::_dispatch_event(XEvent &event) {}

    std::shared_ptr<window> x11_system::create_window(const int width, const int height, const std::string_view title) {
        return std::make_shared<x11_window>(std::static_pointer_cast<x11_system>(shared_from_this()), width, height, title);
    }

    std::variant<bool, std::monostate> x11_system::get_presentation_support(const vk::raii::Instance &instance, const vk::PhysicalDevice physical_device,
                                                                            const uint32_t queue_family) const {
        PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR getSupport = (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR)(instance.getProcAddr(
            "vkGetPhysicalDeviceXlibPresentationSupportKHR"));
        return getSupport(physical_device, queue_family, _display, XVisualIDFromVisual(DefaultVisual(_display, _default_screen))) == vk::True;
    }

    x11_window::x11_window(const std::shared_ptr<x11_system> &system_, int width, int height, const std::string_view title)
        : window::window(std::static_pointer_cast<system>(system_)) {
        XSetWindowAttributes attr{};
        attr.event_mask = StructureNotifyMask;
        _window         = XCreateWindow(system_->_display,
                                system_->_root,
                                0,
                                0,
                                width,
                                height,
                                0,
                                DefaultDepth(system_->_display, system_->_default_screen),
                                InputOutput,
                                DefaultVisual(system_->_display, system_->_default_screen),
                                CWEventMask,
                                &attr);
        XMapWindow(system_->_display, _window);

        XPending(system_->_display);
    }

    x11_window::~x11_window() {
        XDestroyWindow(std::reinterpret_pointer_cast<x11_system>(_system)->_display, _window);
    }

    bool x11_window::should_close() const {
        return _close_requested;
    }

    window_size_t x11_window::size() const {
        XWindowAttributes wa{};
        XGetWindowAttributes(std::reinterpret_pointer_cast<x11_system>(_system)->_display, _window, &wa);
        return {wa.width, wa.height};
    }

    void x11_window::set_title(const std::string_view title) {
        XStoreName(std::reinterpret_pointer_cast<x11_system>(_system)->_display, _window, title.data());
    }

    vk::raii::SurfaceKHR x11_window::create_surface(const vk::raii::Instance &instance) {
        vk::XlibSurfaceCreateInfoKHR sci{};
        sci.window                      = _window;
        sci.dpy                         = std::reinterpret_pointer_cast<x11_system>(_system)->_display;
        VkXlibSurfaceCreateInfoKHR sci_ = sci;

        PFN_vkCreateXlibSurfaceKHR createSurf = (PFN_vkCreateXlibSurfaceKHR)(instance.getProcAddr("vkCreateXlibSurfaceKHR"));
        VkSurfaceKHR               s;
        createSurf(*instance, &sci_, nullptr, &s);
        return vk::raii::SurfaceKHR(instance, s);
    }
} // namespace neuron::window

#endif
