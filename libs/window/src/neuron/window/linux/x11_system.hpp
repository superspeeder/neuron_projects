//
// Created by andy on 11/17/25.
//

#pragma once
#if defined(NEURON_WINDOW_TARGET_LINUX) && defined(NEURON_WINDOW_X11_SUPPORT)
#include "./linux.hpp"

#include <X11/Xlib.h>

namespace neuron::window {
    class x11_system : public linux_system {
    public:
        x11_system() = default;
        ~x11_system() override = default;

        const std::vector<const char *> &required_instance_extensions() override;

      private:
        Display* _display;
        int _default_screen;
        Window _root;
    };
}
#endif
