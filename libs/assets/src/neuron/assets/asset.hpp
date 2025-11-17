//
// Created by andy on 11/16/25.
//

#pragma once
#include "neuron/uuid.hpp"

namespace neuron::assets {
    class asset {
      public:
        inline asset(const uint32_t id, const uuid& uuid) : _uuid(uuid), _id(id) {}

        virtual ~asset();

        asset(const asset &other)                = delete;
        asset(asset &&other) noexcept            = delete;
        asset &operator=(const asset &other)     = delete;
        asset &operator=(asset &&other) noexcept = delete;

        [[nodiscard]] const uuid& uuid() const noexcept {
            return _uuid;
        }

        [[nodiscard]] uint32_t id() const noexcept {
            return _id;
        }

      private:
        neuron::uuid     _uuid;
        uint32_t _id;
    };

    template<class T>
    concept asset_load_text = std::derived_from<asset, T> && requires (const std::string_view text, const uint32_t id, const neuron::uuid& uuid)
    {
        { T::load_from_text(id, uuid, text) } -> std::same_as<T*>;
    };

    template<class T>
    concept asset_load_binary = std::derived_from<asset, T> && requires (const std::span<uint8_t>& binary, const uint32_t id, const neuron::uuid& uuid)
    {
        { T::load_from_binary(id, uuid, binary) } -> std::same_as<T*>;
    };
} // namespace neuron::assets
