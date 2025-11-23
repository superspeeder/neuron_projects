#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <concepts>
#include <functional>
#include <memory>
#include <optional>

#include <neuron/render_interface/interface.hpp>

namespace neuron::render {
    class vulkan_context {
      public:
        using device_selector_t = std::function<const vk::raii::PhysicalDevice &(const std::vector<vk::raii::PhysicalDevice> &)>;

        struct setup_options {
            std::optional<device_selector_t> _device_selector;

            std::vector<const char *> device_extensions;

            template <class F>
            setup_options &device_selector(F f)
                requires(std::is_invocable_r_v<const vk::raii::PhysicalDevice &, F, const std::vector<vk::raii::PhysicalDevice> &>)
            {
                if constexpr (std::is_assignable_v<device_selector_t, F>) {
                    _device_selector = f;
                } else {
                    _device_selector = [f = std::move(f)](const std::vector<vk::raii::PhysicalDevice> &physical_devices) -> const vk::raii::PhysicalDevice & {
                        return f(physical_devices);
                    };
                }
            };
        };

        explicit vulkan_context(const std::shared_ptr<render_interface::instance_extension_requirement_provider> &ext_provider, const setup_options &options = {});
        ~vulkan_context() = default;

        [[nodiscard]] inline const vk::raii::Context        &context() const { return _context; };
        [[nodiscard]] inline const vk::raii::Instance       &instance() const { return _instance; };
        [[nodiscard]] inline const vk::raii::PhysicalDevice &physical_device() const { return _physical_device; };
        [[nodiscard]] inline const vk::raii::Device         &device() const { return _device; };
        [[nodiscard]] inline const vk::raii::Queue          &queue() const { return _queue; };

        [[nodiscard]] vk::raii::Semaphore      create_semaphore() const;
        [[nodiscard]] vk::raii::Fence          create_fence(vk::FenceCreateFlags flags = {}) const;
        [[nodiscard]] vk::raii::CommandPool    create_command_pool(uint32_t family) const;
        [[nodiscard]] vk::raii::CommandBuffers allocate_command_buffers(const vk::raii::CommandPool &pool, uint32_t count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;

        void wait_fence(vk::Fence fence) const;
        void wait_fence(const vk::raii::Fence &fence) const;

      private:
        vk::raii::Context        _context{};
        vk::raii::Instance       _instance{nullptr};
        vk::raii::PhysicalDevice _physical_device{nullptr};
        vk::raii::Device         _device{nullptr};
        vk::raii::Queue          _queue{nullptr};
    };
} // namespace neuron::render
