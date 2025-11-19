//
// Created by andy on 11/16/25.
//
#include "uuid.hpp"

#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <combaseapi.h>

static neuron::uuid generate_uuid() {
    neuron::uuid val{};
    GUID         guid{};
    if (FAILED(CoCreateGuid(&guid))) {
        throw std::runtime_error("Failed to create uuid.");
    }
    return std::bit_cast<neuron::uuid>(guid);
}

static constexpr char hex_chars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

inline void write_byte_hex(unsigned char b, char* buf) {
    *buf = hex_chars[b >> 4];
    *(buf + 1) = hex_chars[b & 0xf];
}

static void write_uuid_string(const neuron::uuid &u, char *buf) {
    for (uint32_t i = 0 ; i < 4 ; i++) {
        write_byte_hex(u.data[i], buf);
        buf += 2;
    }
    *(buf++) = '-';
    for (uint32_t i = 0 ; i < 2 ; i++) {
        write_byte_hex(u.data[i], buf);
        buf += 2;
    }
    *(buf++) = '-';
    for (uint32_t i = 0 ; i < 2 ; i++) {
        write_byte_hex(u.data[i], buf);
        buf += 2;
    }
    *(buf++) = '-';
    for (uint32_t i = 0 ; i < 8 ; i++) {
        write_byte_hex(u.data[i], buf);
        buf += 2;
    }
    *buf = 0;
}


#elif defined(__linux__)
#include <uuid/uuid.h>

static neuron::uuid generate_uuid() {
    neuron::uuid val{};
    uuid_generate_random(val.data);
    return val;
}

static void write_uuid_string(const neuron::uuid &u, char *buf) {
    uuid_unparse_lower(u.data, buf);
}

#endif

namespace neuron {
    uuid uuid::generate_v4() {
        return generate_uuid();
    }

    std::string uuid::to_string() const {
        char buf[37];
        buf[36] = 0;
        write_uuid_string(*this, buf);
        return buf;
    }

    std::ostream &operator<<(std::ostream &os, const uuid &u) {
        char buf[37];
        buf[36] = 0;
        write_uuid_string(u, buf);
        os << buf;
        return os;
    }
} // namespace neuron
