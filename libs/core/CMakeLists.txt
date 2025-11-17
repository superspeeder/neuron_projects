cmake_minimum_required(VERSION 3.30)
project(neuron_core)

add_library(neuron_core src/neuron/core.cpp src/neuron/core.hpp src/neuron/sparse_storage.hpp
        src/neuron/worker_pool.hpp
        src/neuron/worker_control_sync.hpp
        src/neuron/uuid.hpp
        src/neuron/uuid.cpp)
add_library(neuron::core ALIAS neuron_core)

target_include_directories(neuron_core PUBLIC src)
target_compile_features(neuron_core PUBLIC cxx_std_23)

if (LINUX)
    target_link_libraries(neuron_core PUBLIC uuid)
endif()