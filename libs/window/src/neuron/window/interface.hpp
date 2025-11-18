//
// Created by andy on 11/17/25.
//

#pragma once
#include <memory>
#include <vector>

namespace neuron::window {
    class system : public std::enable_shared_from_this<system> {
    public:
        system() = default;
        virtual ~system() = default;

        system(const system&) = delete;
        system(system&&) = delete;
        system& operator=(const system&) = delete;
        system& operator=(system&&) = delete;

        virtual const std::vector<const char *> & required_instance_extensions() = 0;
        virtual void poll() = 0;
    };
}