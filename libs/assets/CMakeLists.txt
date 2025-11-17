cmake_minimum_required(VERSION 3.30)
project(neuron_assets)

add_library(neuron_assets src/neuron/assets/asset.hpp
        src/neuron/assets/types/text_asset.hpp)
target_include_directories(neuron_assets PUBLIC src/)
target_link_libraries(neuron_assets PUBLIC neuron_core)
target_compile_features(neuron_assets PUBLIC cxx_std_23)

add_library(neuron::assets ALIAS neuron_assets)