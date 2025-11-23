//
// Created by andy on 11/15/25.
//

#include "neuron/render/surface.hpp"
#include "neuron/render/swapchain.hpp"


#include <neuron/uuid.hpp>
#include <neuron/window/window.hpp>

#include <neuron/sparse_storage.hpp>

#include <chrono>
#include <print>
#include <vulkan/vulkan_raii.hpp>

#include <neuron/render/render.hpp>

template <class... Ts>
struct overloads : Ts... {
    using Ts::operator()...;
};

struct testing_app_state {
    std::shared_ptr<neuron::window::system>         winsys;
    std::shared_ptr<neuron::window::window>         window;
    std::shared_ptr<neuron::render::vulkan_context> vulkan_context;
    std::shared_ptr<neuron::render::surface>        surface;
    std::shared_ptr<neuron::render::swapchain>      swapchain;

    vk::raii::Semaphore semaphore{nullptr};
    vk::raii::Fence     fence{nullptr};

    bool running = true;

    testing_app_state() {
        winsys = neuron::window::create_system();
        window = winsys->create_window(800, 600, "Neuron Example App");

        vulkan_context = std::make_shared<neuron::render::vulkan_context>(winsys);

        surface   = std::make_shared<neuron::render::surface>(vulkan_context, window);
        fence     = vulkan_context->create_fence(vk::FenceCreateFlagBits::eSignaled);
        semaphore = vulkan_context->create_semaphore();

        swapchain = std::make_shared<neuron::render::swapchain>(surface, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst);

        window->set_on_redraw_callback([&] { update(); });
    }

    neuron::window::window_size_t swapchain_size{};

    void recreate_swapchain() {
        vulkan_context->device().waitIdle();
        swapchain->refresh(window->size());
        draw_images();
    }

    struct SimpleImageMode {
        vk::ImageLayout layout;
        vk::AccessFlags access;
    };

    static constexpr SimpleImageMode UNDEFINED_MODE{vk::ImageLayout::eUndefined, vk::AccessFlagBits::eNone};
    static constexpr SimpleImageMode TRANSFER_WRITE_MODE{vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eTransferWrite};
    static constexpr SimpleImageMode PRESENT_MODE{vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits::eNone};

    static vk::ImageMemoryBarrier simple_imb(const vk::Image image, const SimpleImageMode &old_mode, const SimpleImageMode &new_mode) {
        vk::ImageMemoryBarrier imb{};
        imb.image            = image;
        imb.oldLayout        = old_mode.layout;
        imb.newLayout        = new_mode.layout;
        imb.srcAccessMask    = old_mode.access;
        imb.dstAccessMask    = new_mode.access;
        imb.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

        return imb;
    }

    void draw_images() {
        const vk::raii::CommandPool    pool = vulkan_context->create_command_pool(0);
        const vk::raii::CommandBuffers cmds = vulkan_context->allocate_command_buffers(pool, 1);

        auto &cmd = cmds[0];
        cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        const auto &images = swapchain->images();
        for (const auto &image : images) {
            auto imb = simple_imb(image, UNDEFINED_MODE, TRANSFER_WRITE_MODE);
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, imb);
        }

        for (const auto &image : images) {
            cmd.clearColorImage(image,
                                vk::ImageLayout::eTransferDstOptimal,
                                vk::ClearColorValue(0.0f, 0.0f, 1.0f, 1.0f),
                                {vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)});
        }

        for (const auto &image : images) {
            auto imb = simple_imb(image, TRANSFER_WRITE_MODE, PRESENT_MODE);
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, imb);
        }
        cmd.end();
        auto fence_ = vulkan_context->create_fence();

        vk::SubmitInfo si{};
        si.setCommandBuffers(*cmd);

        vulkan_context->queue().submit(si, fence_);
        auto _ = vulkan_context->device().waitForFences(*fence_, true, UINT64_MAX);
    }

    void mainloop() {
        printf("\n");
        while (running && !window->should_close()) {
            winsys->poll();
            window->request_redraw();
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

        auto _ = vulkan_context->device().waitForFences(*fence, true, UINT64_MAX);
        vulkan_context->device().resetFences(*fence);
        if (swapchain_size != window->size()) {
            recreate_swapchain();
        }


        try {
            auto [index, suboptimal] = swapchain->acquire_next_image(*semaphore, *fence);
            suboptimal               = swapchain->present(index, semaphore) || suboptimal;

            if (suboptimal) {
                recreate_swapchain();
            }
        } catch (vk::OutOfDateKHRError &e) {
            recreate_swapchain();
        }

        last_frame = this_frame;
        this_frame = clock::now();
    }

    testing_app_state(const testing_app_state &other)                = delete;
    testing_app_state(testing_app_state &&other) noexcept            = delete;
    testing_app_state &operator=(const testing_app_state &other)     = delete;
    testing_app_state &operator=(testing_app_state &&other) noexcept = delete;

    ~testing_app_state() { vulkan_context->device().waitIdle(); }
};


int main() {
    testing_app_state state{};
    state.mainloop();

    return 0;
}
