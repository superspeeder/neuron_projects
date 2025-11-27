//
// Created by andy on 11/22/2025.
//

#include "surface_renderer.hpp"

#include <iostream>

namespace neuron::render {
    surface_renderer::surface_renderer(const std::shared_ptr<vulkan_context> &context, const std::shared_ptr<render_interface::surface_provider> &surface_provider,
                                       vk::ImageUsageFlags usage)
        : _context(context), _swapchain(std::make_shared<swapchain>(std::make_shared<surface>(context, surface_provider), usage)), _command_pool(context->create_command_pool(0)) {
        for (uint32_t i = 0; i < max_frames_in_flight; i++) {
            _syncs.push_back(std::make_unique<sync_resources>(_context));
        }

        _command_buffers = _context->allocate_command_buffers(_command_pool, max_frames_in_flight);

        create_image_views();
        _swapchain->on_recreated += {
            +[](void *userdata, swapchain *swc) {
                auto *sr = static_cast<surface_renderer *>(userdata);
                sr->create_image_views();
            },
            this,
        };
    }

    surface_renderer::surface_renderer(const std::shared_ptr<vulkan_context> &context, const std::shared_ptr<surface> &surface, vk::ImageUsageFlags usage)
        : _context(context), _swapchain(std::make_shared<swapchain>(surface, usage)), _command_pool(context->create_command_pool(0)) {
        for (uint32_t i = 0; i < max_frames_in_flight; i++) {
            _syncs.push_back(std::make_unique<sync_resources>(_context));
        }

        _command_buffers = _context->allocate_command_buffers(_command_pool, max_frames_in_flight);

        create_image_views();
        _swapchain->on_recreated += {
            +[](void *userdata, swapchain *swc) {
                auto *sr = static_cast<surface_renderer *>(userdata);
                sr->create_image_views();
            },
            this,
        };
    }


    void surface_renderer::render_with(const std::shared_ptr<renderer_base> &renderer) {
        const auto &sync = *_syncs[_current_frame];
        _context->wait_fence(sync.in_flight);

        if (_swapchain->mismatched_extent()) {
            _swapchain->refresh();
        }

        const auto &cmd = _command_buffers[_current_frame];

        const auto [image_index, suboptimal] = _swapchain->acquire_next_image(*sync.image_available, nullptr);
        const frame_resources fr{
            .sync        = sync,
            .cmd         = cmd,
            .image       = _swapchain->images()[image_index],
            .image_index = image_index,
            .extent      = _swapchain->extent(),
            .format      = _swapchain->format(),
            .image_view  = *_image_views[image_index],
        };

        _context->reset_fence(sync.in_flight);

        cmd.reset();
        cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        renderer->pre_render_frame(fr);
        renderer->render_frame(fr);
        renderer->post_render_frame(fr);
        cmd.end();

        vk::SubmitInfo2             si{};
        vk::CommandBufferSubmitInfo cmdsi{};
        cmdsi.setCommandBuffer(*cmd);

        vk::SemaphoreSubmitInfo waitsi{};
        waitsi.setSemaphore(*sync.image_available);
        waitsi.setStageMask(vk::PipelineStageFlagBits2::eTopOfPipe);

        vk::SemaphoreSubmitInfo signalsi{};
        signalsi.setSemaphore(*sync.render_finished);
        signalsi.setStageMask(vk::PipelineStageFlagBits2::eBottomOfPipe);

        si.setCommandBufferInfos(cmdsi);
        si.setWaitSemaphoreInfos(waitsi);
        si.setSignalSemaphoreInfos(signalsi);

        _context->queue().submit2(si, sync.in_flight);

        // ReSharper disable once CppTooWideScopeInitStatement
        const auto recreate = _swapchain->present(image_index, *sync.render_finished);
        if (recreate || suboptimal) {
            _swapchain->refresh();
        }

        _current_frame = (_current_frame + 1) % max_frames_in_flight;
    }
    void surface_renderer::create_image_views() {
        _image_views.clear();
        const auto &format = _swapchain->format();
        for (const auto &image : _swapchain->images()) {
            _image_views.emplace_back(context->device(),
                                      vk::ImageViewCreateInfo{
                                          {},
                                          image,
                                          vk::ImageViewType::e2D,
                                          format,
                                          vk::ComponentMapping{
                                              vk::ComponentSwizzle::eR,
                                              vk::ComponentSwizzle::eG,
                                              vk::ComponentSwizzle::eB,
                                              vk::ComponentSwizzle::eA,
                                          },
                                          vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
                                      });
        }
    }

    void renderer_base::pre_render_frame(const frame_resources &frame_resources) {
        const auto             &cmd = frame_resources.cmd;
        vk::ImageMemoryBarrier2 imb2{};
        imb2.image            = frame_resources.image;
        imb2.oldLayout        = initial_layout;
        imb2.newLayout        = render_layout;
        imb2.srcAccessMask    = initial_access;
        imb2.dstAccessMask    = render_access;
        imb2.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
        imb2.srcStageMask     = initial_stage;
        imb2.dstStageMask     = render_stage;

        cmd.pipelineBarrier2({{}, {}, {}, imb2});
    }

    void renderer_base::post_render_frame(const frame_resources &frame_resources) {
        const auto             &cmd = frame_resources.cmd;
        vk::ImageMemoryBarrier2 imb2{};
        imb2.image            = frame_resources.image;
        imb2.oldLayout        = render_layout;
        imb2.newLayout        = final_layout;
        imb2.srcAccessMask    = render_access;
        imb2.dstAccessMask    = final_access;
        imb2.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
        imb2.srcStageMask     = render_stage;
        imb2.dstStageMask     = final_stage;

        cmd.pipelineBarrier2({{}, {}, {}, imb2});
    }

    void dynamic_rendering::pre_render_frame(const frame_resources &frame_resources, const renderer_base *renderer) {
        const auto                 &cmd = frame_resources.cmd;
        vk::RenderingAttachmentInfo color_attachment{
            frame_resources.image_view,
            renderer->render_layout,
            vk::ResolveModeFlagBits::eNone,
            {},
            vk::ImageLayout::eUndefined,
            load_op,
            store_op,
            clear_color,
        };
        cmd.beginRendering(vk::RenderingInfo({}, vk::Rect2D({0, 0}, frame_resources.extent), 1, 0, color_attachment));
    }

    void dynamic_rendering::post_render_frame(const frame_resources &frame_resources, const renderer_base *renderer) {
        const auto                 &cmd = frame_resources.cmd;
        cmd.endRendering();
    }
} // namespace neuron::render
