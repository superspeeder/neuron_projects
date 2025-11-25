//
// Created by andy on 11/24/2025.
//

#include "window.hpp"

namespace neuron::window {
    system* global;

    system::~system() {
        if (global == this) global = nullptr;
    }
}