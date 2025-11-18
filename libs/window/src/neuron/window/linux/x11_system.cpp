//
// Created by andy on 11/17/25.
//

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan_raii.hpp>

#include "x11_system.hpp"



namespace neuron::window {
    const std::vector<const char *> &x11_system::required_instance_extensions() {
        static std::vector<const char*> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
        };
        return extensions;
    }
}
