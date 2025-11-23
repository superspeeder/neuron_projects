#include "render.hpp"

#include <unordered_set>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace neuron::render {
    vulkan_context::vulkan_context(const std::shared_ptr<render_interface::instance_extension_requirement_provider> &ext_provider, const setup_options &options) {
        VULKAN_HPP_DEFAULT_DISPATCHER.init();
        {
            vk::ApplicationInfo appInfo{};
            appInfo.apiVersion = vk::ApiVersion13;

            vk::InstanceCreateInfo ici{};

            auto extensions = render_interface::required_instance_extensions(ext_provider);
            ici.setPEnabledExtensionNames(extensions);
            ici.setPApplicationInfo(&appInfo);
            _instance = _context.createInstance(ici);
        }
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*_instance);

        auto physical_devices = _instance.enumeratePhysicalDevices();
        if (options._device_selector.has_value()) {
            _physical_device = options._device_selector.value()(physical_devices);
        } else {
            _physical_device = physical_devices[0];
        }

        {
            std::unordered_set<std::string> device_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
            for (const char *ext : options.device_extensions) {
                device_extensions.insert(ext);
            }

            std::vector<const char *> pde;
            for (const auto &e : device_extensions) {
                pde.push_back(e.c_str());
            }

            vk::DeviceCreateInfo dci{};
            dci.setPEnabledExtensionNames(pde);

            std::vector<vk::DeviceQueueCreateInfo> queueCreateInfo{};
            constexpr float                        priority = 1.0f;
            queueCreateInfo.push_back(vk::DeviceQueueCreateInfo{{}, 0, 1, &priority});
            dci.setQueueCreateInfos(queueCreateInfo);

            vk::PhysicalDeviceFeatures2        features{};
            vk::PhysicalDeviceVulkan13Features v13f{};
            v13f.dynamicRendering = true;
            v13f.synchronization2 = true;
            features.pNext        = &v13f;
            dci.pNext             = &features;

            _device = _physical_device.createDevice(dci);
        }
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*_device);

        _queue = _device.getQueue(0, 0);
    }

    vk::raii::Semaphore vulkan_context::create_semaphore() const {
        return {_device, vk::SemaphoreCreateInfo{}};
    }

    vk::raii::Fence vulkan_context::create_fence(vk::FenceCreateFlags flags) const {
        return {_device, {flags}};
    }

    vk::raii::CommandPool vulkan_context::create_command_pool(uint32_t family) const {
        return {_device, {vk::CommandPoolCreateFlagBits::eResetCommandBuffer, family}};
    }

    vk::raii::CommandBuffers vulkan_context::allocate_command_buffers(const vk::raii::CommandPool &pool, uint32_t count, vk::CommandBufferLevel level) const {
        return {_device, vk::CommandBufferAllocateInfo{*pool, level, count}};
    }

    void vulkan_context::wait_fence(vk::Fence fence) const {
        [[maybe_unused]] auto _ = _device.waitForFences(fence, true, UINT64_MAX);
    }

    void vulkan_context::wait_fence(const vk::raii::Fence &fence) const {
        wait_fence(*fence);
    }

    void vulkan_context::reset_fence(vk::Fence fence) const {
        _device.resetFences(fence);
    }

    void vulkan_context::reset_fence(const vk::raii::Fence &fence) const {
        reset_fence(*fence);
    }


} // namespace neuron::render
