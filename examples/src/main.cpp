//
// Created by andy on 11/15/25.
//

#include "neuron/uuid.hpp"


#include <bit>
#include <iostream>
#include <neuron/sparse_storage.hpp>
#include <print>

#include "neuron/window/linux/wayland_system.hpp"

int main() {
    neuron::sparse_storage<const char*, uint32_t, 1<<16, 1<<16> storage;
    storage.set(0, "Hello from 0x0");
    storage.set(UINT32_MAX, "Hello from UINT32_MAX");
    std::println("0x00000000:\t{}", storage.get(0));
    std::println("0xffffffff:\t{}", storage.get(UINT32_MAX));

    auto uuid = neuron::uuid::generate_v4();
    std::println("Test UUID: {}", uuid);

    std::println("p: {}", (std::size_t)(getenv("WAYLAND_DISPLAY")));

    auto winsys = std::make_shared<neuron::window::wayland_system>();
    auto surface = winsys->create_surface();
    auto xdg_surface = surface->xdg();
    xdg_surface->set_appid("Neuron Example App");
    xdg_surface->set_title("Neuron Example App");

    std::println("Scale Factor: {}", winsys->scale_factor());
    std::println("Output Geometry: ({}, {}) {}x{}", winsys->output_geometry().x, winsys->output_geometry().y, winsys->output_geometry().width, winsys->output_geometry().height);
    std::println("Output Make: {}", winsys->output_geometry().make);
    std::println("Output Model: {}", winsys->output_geometry().model);
    std::println("Output Mode: {}x{} @ {} mHz", winsys->output_mode().width, winsys->output_mode().height, winsys->output_mode().refresh);
    std::println("Output Name: {}", winsys->output_name());
    std::println("{}", winsys->output_description());

    while (winsys->dispatch()) {}

    return 0;
}
