cmake_minimum_required(VERSION 3.30)
project(neuron_events)

add_library(neuron_events src/neuron/events/events.cpp src/neuron/events/events.hpp
        src/neuron/events/signal.cpp
        src/neuron/events/signal.hpp)
target_include_directories(neuron_events PUBLIC src/)
target_compile_features(neuron_events PUBLIC cxx_std_23)
add_library(neuron::events ALIAS neuron_events)
