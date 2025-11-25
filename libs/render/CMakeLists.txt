cmake_minimum_required(VERSION 3.30)
project(neuron_render)

add_library(neuron_render src/neuron/render/render.hpp src/neuron/render/render.cpp
        src/neuron/render/surface.cpp
        src/neuron/render/surface.hpp
        src/neuron/render/swapchain.cpp
        src/neuron/render/swapchain.hpp
        src/neuron/render/surface_renderer.cpp
        src/neuron/render/surface_renderer.hpp
)
target_include_directories(neuron_render PUBLIC src/)
target_link_libraries(neuron_render PUBLIC neuron_core neuron_render_interface)
target_compile_features(neuron_render PUBLIC cxx_std_23)
target_compile_definitions(neuron_render PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

add_library(neuron::render ALIAS neuron_render)
