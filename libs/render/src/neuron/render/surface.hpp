//
// Created by andy on 11/19/25.
//

#pragma once
#include "render.hpp"

#include <vector>

namespace neuron::render {

    class surface {
      public:
        surface(const std::shared_ptr<vulkan_context> &context, const std::shared_ptr<render_interface::surface_provider> &surface_provider)
            : _context(context), _surface(surface_provider->create_surface(_context->instance())) {}

        surface(const surface &other)                = delete;
        surface(surface &&other) noexcept            = delete;
        surface &operator=(const surface &other)     = delete;
        surface &operator=(surface &&other) noexcept = delete;

        [[nodiscard]] std::vector<vk::PresentModeKHR>   supported_present_modes() const;
        [[nodiscard]] std::vector<vk::SurfaceFormatKHR> supported_formats() const;
        [[nodiscard]] vk::SurfaceCapabilitiesKHR        capabilities() const;
        [[nodiscard]] bool                              check_support(uint32_t family) const;

        /**
         * Configure the creation info for a swapchain for this surface.
         *
         * This does *not* set the `imageUsage` or 'oldSwapchain` fields. You must set those yourself.
         *
         * @param create_info The swapchain create info to configure for this surface
         * @return If the caller needs to set the swapchain extent manually (caused by the surface not exposing a valid value for this in the capabilities).
         */
        bool configure_swapchain(vk::SwapchainCreateInfoKHR &create_info) const;

        [[nodiscard]] inline const std::shared_ptr<vulkan_context> &context() const noexcept { return _context; };

      private:
        std::shared_ptr<vulkan_context> _context;
        vk::raii::SurfaceKHR            _surface;
    };

} // namespace neuron::render
