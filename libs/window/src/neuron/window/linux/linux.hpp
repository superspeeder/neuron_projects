//
// Created by andy on 11/17/25.
//

#pragma once
#ifdef NEURON_WINDOW_TARGET_LINUX

#include "../interface.hpp"

namespace neuron::window {
    class linux_system : public system {
    public:
        linux_system();
        ~linux_system() override;

        static std::shared_ptr<system> create_system();
    };
}

#endif