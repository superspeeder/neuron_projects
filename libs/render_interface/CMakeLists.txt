cmake_minimum_required(VERSION 3.30)

find_package(Vulkan REQUIRED)

add_library(neuron_render_interface INTERFACE)
target_include_directories(neuron_render_interface INTERFACE include/)
target_link_libraries(neuron_render_interface INTERFACE Vulkan::Headers)
target_compile_features(neuron_render_interface INTERFACE cxx_std_23)


add_library(neuron::render_interface ALIAS neuron_render_interface)