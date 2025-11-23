cmake_minimum_required(VERSION 3.30)

add_executable(neuron_example src/main.cpp)
target_include_directories(neuron_example PUBLIC src)
target_compile_features(neuron_example PUBLIC cxx_std_23)
target_link_libraries(neuron_example PUBLIC neuron::core neuron::assets neuron::window neuron::render)
