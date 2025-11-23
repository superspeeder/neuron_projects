//
// Created by andy on 11/22/2025.
//

#pragma once

#include "neuron/render/render.hpp"
#include "neuron/render/swapchain.hpp"

#include <neuron/render_interface/interface.hpp>

namespace neuron::render {

    struct sync_resources {
        vk::raii::Semaphore image_available;
        vk::raii::Semaphore render_finished;
        vk::raii::Fence     in_flight;

        explicit sync_resources(const std::shared_ptr<vulkan_context> &context)
            : image_available(context->device(), vk::SemaphoreCreateInfo{}), render_finished(context->device(), vk::SemaphoreCreateInfo{}), in_flight(context->device(), {vk::FenceCreateFlagBits::eSignaled}) {}

        sync_resources(const sync_resources &other)                = delete;
        sync_resources(sync_resources &&other) noexcept            = delete;
        sync_resources &operator=(const sync_resources &other)     = delete;
        sync_resources &operator=(sync_resources &&other) noexcept = delete;
    };

    struct frame_resources {
        const sync_resources          &sync;
        const vk::raii::CommandBuffer &cmd;
        vk::Image                      image;
        uint32_t                       image_index;
        vk::Extent2D                   extent;
        vk::Format                     format;
    };


    class renderer_base;

    class surface_renderer {
      public:
        surface_renderer(const std::shared_ptr<vulkan_context> &context, const std::shared_ptr<render_interface::surface_provider> &surface_provider,
                         vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment);

        surface_renderer(const std::shared_ptr<vulkan_context> &context, const std::shared_ptr<surface> &surface,
                         vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment);

        void render_with(const std::shared_ptr<renderer_base> &renderer);

        static constexpr uint32_t max_frames_in_flight = 2;

      private:
        std::shared_ptr<vulkan_context> _context;
        std::shared_ptr<swapchain>      _swapchain;

        std::vector<std::unique_ptr<sync_resources>> _syncs;
        vk::raii::CommandPool                        _command_pool;
        std::vector<vk::raii::CommandBuffer>         _command_buffers;
        uint32_t                                     _current_frame = 0;
    };


    class renderer_base {
      public:
        renderer_base()                                          = default;
        virtual ~renderer_base()                                 = default;
        renderer_base(const renderer_base &other)                = delete;
        renderer_base(renderer_base &&other) noexcept            = delete;
        renderer_base &operator=(const renderer_base &other)     = delete;
        renderer_base &operator=(renderer_base &&other) noexcept = delete;

        virtual void render_frame(const frame_resources &frame_resources) = 0;

        // default implementations just do image layout transitions.
        virtual void pre_render_frame(const frame_resources &frame_resources);
        virtual void post_render_frame(const frame_resources &frame_resources);

      protected:
        vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined;
        vk::ImageLayout render_layout  = vk::ImageLayout::eColorAttachmentOptimal;
        vk::ImageLayout final_layout   = vk::ImageLayout::ePresentSrcKHR;

        vk::AccessFlags2 initial_access = vk::AccessFlagBits2::eNone;
        vk::AccessFlags2 render_access  = vk::AccessFlagBits2::eColorAttachmentWrite;
        vk::AccessFlags2 final_access   = vk::AccessFlagBits2::eNone;

        vk::PipelineStageFlags2 initial_stage = vk::PipelineStageFlagBits2::eTopOfPipe;
        vk::PipelineStageFlags2 render_stage  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        vk::PipelineStageFlags2 final_stage   = vk::PipelineStageFlagBits2::eBottomOfPipe;

      private:
    };
} // namespace neuron::render
