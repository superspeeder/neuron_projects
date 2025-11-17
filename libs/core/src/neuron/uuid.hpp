//
// Created by andy on 11/16/25.
//

#pragma once
#include <cstdint>
#include <iostream>
#include <format>

namespace neuron {
    struct uuid {
        uint8_t data[16];

        static uuid generate_v4();

        friend std::ostream& operator<<(std::ostream& os, const uuid& u);
        [[nodiscard]] std::string to_string() const;
    };
}

template<> struct std::formatter<neuron::uuid> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const neuron::uuid& u, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}", u.to_string());
    }
};
