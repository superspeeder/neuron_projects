//
// Created by andy on 11/19/25.
//

#include "swapchain.hpp"

#include <iostream>
#include <ostream>

namespace neuron::render {
    swapchain::swapchain(const std::shared_ptr<surface> &surface, vk::ImageUsageFlags image_usage) : _surface(surface), _image_usage(image_usage), _context(surface->context()) {
        _create_swapchain();
    }

    void swapchain::refresh() {
        _context->device().waitIdle();
        _create_swapchain();
    }

    bool swapchain::mismatched_extent() const {
        return _extent != _surface->extent();
    }

    std::pair<uint32_t, bool> swapchain::acquire_next_image(const vk::Semaphore semaphore, const vk::Fence fence) const {
        auto [res, index] = _swapchain.acquireNextImage(UINT64_MAX, semaphore, fence);
        return {index, res == vk::Result::eSuboptimalKHR};
    }

    bool swapchain::present(uint32_t index, vk::Semaphore wait) const {
        try {
            return _context->queue().presentKHR(vk::PresentInfoKHR(wait, *_swapchain, index)) == vk::Result::eSuboptimalKHR;
        } catch (vk::OutOfDateKHRError) {
            return true;
        }
    }

    void swapchain::_create_swapchain() {
        vk::SwapchainCreateInfoKHR sci{};
        sci.imageUsage = _image_usage;

        if (_swapchain != nullptr) {
            sci.oldSwapchain = *_swapchain;
        }

        _surface->configure_swapchain(sci);

        _swapchain = vk::raii::SwapchainKHR(_context->device(), sci);
        _images    = _swapchain.getImages();

        _format       = sci.imageFormat;
        _color_space  = sci.imageColorSpace;
        _extent       = sci.imageExtent;
        _present_mode = sci.presentMode;
    }
} // namespace neuron::render
