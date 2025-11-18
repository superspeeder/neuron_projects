//
// Created by andy on 11/15/25.
//

#include "neuron/uuid.hpp"
#include "neuron/window/window.hpp"

#include <iostream>
#include <neuron/sparse_storage.hpp>
#include <print>

#include <chrono>
#include <vulkan/vulkan_raii.hpp>

struct testing_app_state {
    std::shared_ptr<neuron::window::system> winsys;
    std::shared_ptr<neuron::window::window> window;

    vk::raii::Context        context;
    vk::raii::Instance       instance{nullptr};
    vk::raii::PhysicalDevice gpu{nullptr};
    vk::raii::Device         device{nullptr};
    vk::raii::SurfaceKHR     surface{nullptr};
    vk::raii::SwapchainKHR   swapchain{nullptr};
    std::vector<vk::Image>   images;
    vk::raii::Queue          queue{nullptr};
    vk::raii::Semaphore      semaphore{nullptr};
    vk::raii::Fence          fence{nullptr};

    bool running = true;

    testing_app_state() {
        winsys = neuron::window::create_system();
        window = winsys->create_window(800, 600, "Neuron Example App");

        {
            vk::ApplicationInfo appInfo{};
            appInfo.apiVersion = vk::ApiVersion14;

            vk::InstanceCreateInfo ici{};
            auto                   extensions = winsys->required_instance_extensions();
            ici.setPEnabledExtensionNames(extensions);
            ici.setPApplicationInfo(&appInfo);
            instance = context.createInstance(ici);
        }

        gpu = instance.enumeratePhysicalDevices()[0];

        {
            vk::DeviceCreateInfo      dci{};
            std::vector<const char *> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

            dci.setPEnabledExtensionNames(extensions);

            std::vector<vk::DeviceQueueCreateInfo> queueCreateInfo{};
            constexpr float                        priority = 1.0f;
            queueCreateInfo.push_back(vk::DeviceQueueCreateInfo{{}, 0, 1, &priority});
            dci.setQueueCreateInfos(queueCreateInfo);

            vk::PhysicalDeviceFeatures2        features{};
            vk::PhysicalDeviceVulkan13Features v13f{};
            v13f.dynamicRendering = true;
            features.pNext        = &v13f;
            dci.pNext             = &features;

            device = gpu.createDevice(dci);
        }

        surface   = window->create_surface(instance);
        queue     = device.getQueue(0, 0);
        semaphore = device.createSemaphore({});
        fence     = device.createFence({vk::FenceCreateFlagBits::eSignaled});

        create_swapchain();
    }

    neuron::window::window_size_t swapchain_size;

    void create_swapchain() {
        device.waitIdle();
        vk::SwapchainCreateInfoKHR sci{};
        sci.presentMode      = vk::PresentModeKHR::eFifo;
        sci.surface          = *surface;
        sci.clipped          = true;
        sci.compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        sci.preTransform     = vk::SurfaceTransformFlagBitsKHR::eIdentity;
        sci.imageArrayLayers = 1;
        sci.imageColorSpace  = vk::ColorSpaceKHR::eSrgbNonlinear;
        sci.imageFormat      = vk::Format::eB8G8R8A8Srgb;
        sci.imageSharingMode = vk::SharingMode::eExclusive;
        sci.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
        sci.imageExtent      = window->size();
        sci.minImageCount    = 3;
        if (*swapchain)
            sci.oldSwapchain = *swapchain;

        swapchain_size = sci.imageExtent;

        swapchain = device.createSwapchainKHR(sci);
        images    = swapchain.getImages();

        draw_images();
    }

    void draw_images() {
        const vk::raii::CommandPool    pool(device, vk::CommandPoolCreateInfo({}, 0));
        const vk::raii::CommandBuffers cmds(device, vk::CommandBufferAllocateInfo(*pool, vk::CommandBufferLevel::ePrimary, 1));

        auto &cmd = cmds[0];
        cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        for (const auto &image : images) {
            vk::ImageMemoryBarrier imb{};
            imb.image            = image;
            imb.oldLayout        = vk::ImageLayout::eUndefined;
            imb.newLayout        = vk::ImageLayout::eTransferDstOptimal;
            imb.srcAccessMask    = vk::AccessFlagBits::eNone;
            imb.dstAccessMask    = vk::AccessFlagBits::eTransferWrite;
            imb.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, imb);
        }

        for (const auto &image : images) {
            cmd.clearColorImage(image, vk::ImageLayout::eTransferDstOptimal, vk::ClearColorValue(0.0f, 0.0f, 1.0f, 1.0f),
                                {vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)});
        }

        for (const auto &image : images) {
            vk::ImageMemoryBarrier imb{};
            imb.image            = image;
            imb.oldLayout        = vk::ImageLayout::eTransferDstOptimal;
            imb.newLayout        = vk::ImageLayout::ePresentSrcKHR;
            imb.srcAccessMask    = vk::AccessFlagBits::eTransferWrite;
            imb.dstAccessMask    = vk::AccessFlagBits::eNone;
            imb.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, imb);
        }
        cmd.end();
        vk::raii::Fence fence_ = device.createFence({});

        vk::SubmitInfo si{};
        si.setCommandBuffers(*cmd);

        queue.submit(si, fence_);
        auto _ = device.waitForFences(*fence_, true, UINT64_MAX);
    }

    void mainloop() {
        winsys->poll();
        while (running && !window->should_close()) {
            winsys->poll();
            update();
        }
    }

    bool remake_swapchain = false;

    using clock           = std::chrono::high_resolution_clock;
    using duration        = std::chrono::duration<double>;
    using time_point      = std::chrono::time_point<clock, duration>;
    time_point this_frame = clock::now();
    time_point last_frame = clock::now() - duration(1.0 / 60.0);

    void update() {
        duration    delta = this_frame - last_frame;
        double      fps   = 1.0 / delta.count();
        std::string s     = "Window - " + std::to_string(fps);
        window->set_title(s);
        auto _ = device.waitForFences(*fence, true, UINT64_MAX);
        device.resetFences(*fence);
        auto [result, index] = swapchain.acquireNextImage(UINT64_MAX, semaphore, fence);
        if (result == vk::Result::eErrorOutOfDateKHR) {
            create_swapchain();
            return;
        }
        vk::PresentInfoKHR pi{};
        pi.setImageIndices(index);
        pi.setSwapchains(*swapchain);
        pi.setWaitSemaphores(*semaphore);
        result = queue.presentKHR(pi);
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || swapchain_size != window->size()) {
            create_swapchain();
        }

        last_frame = this_frame;
        this_frame = clock::now();
    }

    testing_app_state(const testing_app_state &other)                = delete;
    testing_app_state(testing_app_state &&other) noexcept            = delete;
    testing_app_state &operator=(const testing_app_state &other)     = delete;
    testing_app_state &operator=(testing_app_state &&other) noexcept = delete;

    ~testing_app_state() { device.waitIdle(); }
};


int main() {
    testing_app_state state{};
    state.mainloop();

    return 0;
}
