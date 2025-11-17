//
// Created by andy on 11/15/25.
//

#include "neuron/uuid.hpp"


#include <bit>
#include <iostream>
#include <neuron/sparse_storage.hpp>
#include <print>

int main() {
    neuron::sparse_storage<const char*, uint32_t, 1<<16, 1<<16> storage;
    storage.set(0, "Hello from 0x0");
    storage.set(UINT32_MAX, "Hello from UINT32_MAX");
    std::println("0x00000000:\t{}", storage.get(0));
    std::println("0xffffffff:\t{}", storage.get(UINT32_MAX));

    auto uuid = neuron::uuid::generate_v4();
    std::println("Test UUID: {}", uuid);

    return 0;
}
