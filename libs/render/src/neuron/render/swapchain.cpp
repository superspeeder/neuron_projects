//
// Created by andy on 11/19/25.
//

#include "swapchain.hpp"

namespace neuron::render {
    swapchain::swapchain(const std::shared_ptr<surface> &surface, vk::ImageUsageFlags image_usage) : _surface(surface), _image_usage(image_usage), _context(surface->context()) {}

    void swapchain::refresh(const vk::Extent2D& extent) {
        _create_swapchain(extent);
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

    void swapchain::_create_swapchain(const vk::Extent2D& extent) {
        vk::SwapchainCreateInfoKHR sci{};
        sci.imageUsage = _image_usage;

        if (_swapchain != nullptr) {
            sci.oldSwapchain = *_swapchain;
        }

        if (_surface->configure_swapchain(sci)) {
            sci.imageExtent = extent;
        }

        _swapchain = vk::raii::SwapchainKHR(_context->device(), sci);
        _images    = _swapchain.getImages();

        _format       = sci.imageFormat;
        _color_space  = sci.imageColorSpace;
        _extent       = sci.imageExtent;
        _present_mode = sci.presentMode;
    }
} // namespace neuron::render
