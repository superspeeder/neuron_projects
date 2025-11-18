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


#ifdef NEURON_WINDOW_TARGET_WINDOWS
#error "Windows support not yet implemented"
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
#endif