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
    /**
     * @brief Only use this if you know what you are doing. Creates a new system instance, does not wire it up as the global system.
     *
     * You should use init_system most of the time.
     *
     * @return
     */
    inline std::shared_ptr<system> create_system();


    /**
     * @brief This is the intended system init function.
     *
     * This creates a new system instance and sets it as the global system.
     *
     * The caller must make sure that this doesn't go out of scope
     *
     * @return
     */
    inline std::shared_ptr<system> init_system() {
        auto system = create_system();
        global = system.get();
        return system;
    }

    inline std::shared_ptr<window> create_window(const int width, const int height, const std::string_view title) {
        return global->create_window(width, height, title);
    }

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
