//
// Created by andy on 11/19/25.
//

#include "surface.hpp"

namespace neuron::render {
    std::vector<vk::PresentModeKHR> surface::supported_present_modes() const {
        return _context->physical_device().getSurfacePresentModesKHR(*_surface);
    }

    std::vector<vk::SurfaceFormatKHR> surface::supported_formats() const {
        return _context->physical_device().getSurfaceFormatsKHR(*_surface);
    }

    vk::SurfaceCapabilitiesKHR surface::capabilities() const {
        return _context->physical_device().getSurfaceCapabilitiesKHR(*_surface);
    }

    bool surface::check_support(const uint32_t family) const {
        return _context->physical_device().getSurfaceSupportKHR(family, *_surface);
    }

    bool surface::configure_swapchain(vk::SwapchainCreateInfoKHR &create_info) const {
        auto formats = supported_formats();
        // auto present_modes = supported_present_modes();
        auto caps = capabilities();

        auto min_image_count = caps.minImageCount + 1;
        if (caps.maxImageCount > 0 && caps.maxImageCount < min_image_count) {
            min_image_count = caps.maxImageCount;
        }

        auto format = formats[0];
        for (const auto &fmt : formats) {
            if (fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear && fmt.format == vk::Format::eB8G8R8A8Srgb) {
                format = fmt;
            }
        }

        create_info.clipped = true;

        create_info.imageArrayLayers = 1;
        create_info.imageColorSpace  = format.colorSpace;
        create_info.imageFormat      = format.format;
        create_info.imageSharingMode = vk::SharingMode::eExclusive;
        create_info.imageExtent      = caps.currentExtent;
        create_info.minImageCount    = min_image_count;

        create_info.pQueueFamilyIndices   = nullptr;
        create_info.queueFamilyIndexCount = 0;
        create_info.compositeAlpha        = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        create_info.preTransform          = caps.currentTransform;
        create_info.presentMode           = vk::PresentModeKHR::eFifo;
        create_info.surface               = *_surface;

        return caps.currentExtent.width == UINT32_MAX;
    }
} // namespace neuron::render
