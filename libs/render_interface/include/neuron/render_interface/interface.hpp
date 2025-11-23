#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <concepts>
#include <string>
#include <vector>

namespace neuron::render_interface {
    class surface_provider {
      public:
        virtual ~surface_provider() = default;

        virtual vk::raii::SurfaceKHR create_surface(const vk::raii::Instance &instance) = 0;
    };

    struct instance_extension {};
    struct device_extension {};

    template <typename T>
    concept extension_type = std::same_as<T, instance_extension> || std::same_as<T, device_extension>;

    template <extension_type E>
    class extension_requirement_provider {
      public:
        virtual ~extension_requirement_provider()                             = default;
        virtual const std::vector<const char *> &required_extensions(E) const = 0;

      private:
    };

    template <std::derived_from<extension_requirement_provider<instance_extension>> T>
    const std::vector<const char *> &required_instance_extensions(const T &v) {
        return v.required_extensions(instance_extension{});
    }


    template <std::derived_from<extension_requirement_provider<device_extension>> T>
    const std::vector<const char *> &required_device_extensions(const T &v) {
        return v.required_extensions(device_extension{});
    }

    template <typename T, typename E>
    concept extension_type_type_p = extension_type<E> && !std::derived_from<T, extension_requirement_provider<instance_extension>> &&
        requires(const T &v, E e) { std::derived_from<std::remove_reference_t<decltype(*v)>, extension_requirement_provider<E>>; };

    template <extension_type_type_p<instance_extension> T>
    const std::vector<const char *> &required_instance_extensions(const T &v) {
        return v->required_extensions(instance_extension{});
    }

    template <extension_type_type_p<device_extension> T>
    const std::vector<const char *> &required_device_extensions(const T &v) {
        return v.required_extensions(device_extension{});
    }

    using instance_extension_requirement_provider = extension_requirement_provider<render_interface::instance_extension>;
    using device_extension_requirement_provider   = extension_requirement_provider<render_interface::device_extension>;
} // namespace neuron::render_interface
