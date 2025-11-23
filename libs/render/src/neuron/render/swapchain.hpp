//
// Created by andy on 11/19/25.
//

#pragma once

#include "neuron/render/render.hpp"
#include "neuron/render/surface.hpp"

namespace neuron::render {

    class swapchain {
      public:
        explicit swapchain(const std::shared_ptr<surface> &surface, vk::ImageUsageFlags image_usage = vk::ImageUsageFlagBits::eColorAttachment);
        ~swapchain() = default;

        swapchain(const swapchain &other)                = delete;
        swapchain(swapchain &&other) noexcept            = delete;
        swapchain &operator=(const swapchain &other)     = delete;
        swapchain &operator=(swapchain &&other) noexcept = delete;

        void refresh(const vk::Extent2D& extent);


        /**
         * Acquire the next image
         *
         * @param semaphore Semaphore to signal after image is available
         * @param fence Fence to signal after image is available
         * @return [image_index, suboptimal]
         * @throws vk::OutOfDateKHRError when swapchain must be recreated before presenting another frame.
         */
        std::pair<uint32_t, bool> acquire_next_image(vk::Semaphore semaphore, vk::Fence fence = {}) const;

        bool present(uint32_t index, vk::Semaphore wait) const;

        [[nodiscard]] const std::vector<vk::Image> &images() const { return _images; }

      private:
        std::shared_ptr<vulkan_context> _context;
        std::shared_ptr<surface>        _surface;
        vk::raii::SwapchainKHR          _swapchain{nullptr};
        vk::ImageUsageFlags             _image_usage;
        vk::Format                      _format;
        vk::ColorSpaceKHR               _color_space;
        vk::PresentModeKHR              _present_mode;
        vk::Extent2D                    _extent;
        std::vector<vk::Image>          _images;

        void _create_swapchain(const vk::Extent2D& extent);
    };

} // namespace neuron::render
