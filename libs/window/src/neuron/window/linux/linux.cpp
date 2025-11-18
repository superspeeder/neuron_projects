//
// Created by andy on 11/17/25.
//

#include "./linux.hpp"

#include <cstring>

#include "wayland_system.hpp"
#include "x11_system.hpp"

namespace neuron::window {
    linux_system::linux_system() = default;
    linux_system::~linux_system() = default;

    std::shared_ptr<system> linux_system::create_system() {
#ifdef NEURON_WINDOW_WAYLAND_SUPPORT
        if (const char *wayland_display = getenv("WAYLAND_DISPLAY"); wayland_display != nullptr) {
            return std::make_shared<wayland_system>();
        }
#endif

#ifdef NEURON_WINDOW_X11_SUPPORT
        if (const char *xorg_display = getenv("DISPLAY"); xorg_display != nullptr) {
            return std::make_shared<x11_system>();
        }
#endif

        throw std::runtime_error("No supported display manager is running currently (or neuron::window hasn't been compiled with support for the running display manager).");
    }
} // namespace neuron::window
