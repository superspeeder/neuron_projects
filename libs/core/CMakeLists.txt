cmake_minimum_required(VERSION 3.30)
project(neuron_core)

add_library(neuron_core src/neuron/core.cpp src/neuron/core.hpp src/neuron/sparse_storage.hpp
        src/neuron/worker_pool.hpp
        src/neuron/worker_control_sync.hpp)
add_library(neuron::core ALIAS neuron_core)

target_include_directories(neuron_core PUBLIC src)
target_compile_features(neuron_core PUBLIC cxx_std_23)