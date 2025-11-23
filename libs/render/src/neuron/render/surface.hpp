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
            : _context(context), _surface(surface_provider->create_surface(_context->instance())), _surface_provider(surface_provider) {}

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
         */
        void configure_swapchain(vk::SwapchainCreateInfoKHR &create_info) const;

        [[nodiscard]] inline const std::shared_ptr<vulkan_context> &context() const noexcept { return _context; }

        vk::Extent2D extent() const;

      private:
        std::shared_ptr<vulkan_context>                     _context;
        vk::raii::SurfaceKHR                                _surface;
        std::shared_ptr<render_interface::surface_provider> _surface_provider;
    };

} // namespace neuron::render
