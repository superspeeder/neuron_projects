//
// Created by andy on 11/16/25.
//

#pragma once
#include <utility>

#include "neuron/assets/asset.hpp"

namespace neuron::assets {
    class text_asset : public asset {
      public:
        text_asset(std::string text, const uint32_t id, const neuron::uuid &uuid) : asset(id, uuid), _text(std::move(text)) {}

        [[nodiscard]] const std::string& text() const noexcept { return _text; }

        static text_asset* load_from_text(const uint32_t id, const neuron::uuid &uuid, const std::string& text) {
            return new text_asset(text, id, uuid);
        }

      private:
        std::string _text;
    };
} // namespace neuron::assets
