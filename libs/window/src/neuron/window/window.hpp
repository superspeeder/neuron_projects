//
// Created by andy on 11/17/25.
//

#pragma once

#if defined(NEURON_WINDOW_TARGET_WINDOWS) && defined(NEURON_WINDOW_TARGET_LINUX)
#error "Cannot build for multiple target operating systems at once."
#endif

#if !defined(NEURON_WINDOW_TARGET_WINDOWS) && !defined(NEURON_WINDOW_TARGET_LINUX)
#error "Cannot build for no target systems."
#endif


#include "interface.hpp"

#ifdef NEURON_WINDOW_TARGET_WINDOWS
#include "neuron/window/windows/windows.hpp"
#endif

#ifdef NEURON_WINDOW_TARGET_LINUX
#include "neuron/window/linux/linux.hpp"
#endif

namespace neuron::window {
    inline std::shared_ptr<system> create_system();
}


#if defined(NEURON_WINDOW_TARGET_LINUX)
inline std::shared_ptr<neuron::window::system> neuron::window::create_system() {
    return linux_system::create_system();
}
#elif defined(NEURON_WINDOW_TARGET_WINDOWS)
inline std::shared_ptr<neuron::window::system> neuron::window::create_system() {
    return std::make_shared<windows_system>();
}
#endif
