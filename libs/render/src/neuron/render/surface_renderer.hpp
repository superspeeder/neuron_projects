//
// Created by andy on 11/22/2025.
//

#pragma once

#include "neuron/render/render.hpp"
#include "neuron/render/surface_renderer.hpp"
#include "neuron/render/swapchain.hpp"

#include <neuron/render_interface/interface.hpp>

namespace neuron::render {

    struct sync_resources {
        vk::raii::Semaphore image_available;
        vk::raii::Semaphore render_finished;
        vk::raii::Fence     in_flight;

        explicit sync_resources(const std::shared_ptr<vulkan_context> &context)
            : image_available(context->device(), vk::SemaphoreCreateInfo{}), render_finished(context->device(), vk::SemaphoreCreateInfo{}),
              in_flight(context->device(), {vk::FenceCreateFlagBits::eSignaled}) {}

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
        vk::ImageView                  image_view;
    };


    class renderer_base;

    class surface_renderer {
      public:
        surface_renderer(const std::shared_ptr<vulkan_context> &context, const std::shared_ptr<render_interface::surface_provider> &surface_provider,
                         vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment);

        surface_renderer(const std::shared_ptr<vulkan_context> &context, const std::shared_ptr<surface> &surface,
                         vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment);

        inline static std::shared_ptr<surface_renderer> create(const std::shared_ptr<render_interface::surface_provider> &surface_provider,
                                                               vk::ImageUsageFlags                                        usage = vk::ImageUsageFlagBits::eColorAttachment) {
            return std::make_shared<surface_renderer>(context->shared_from_this(), surface_provider, usage);
        }

        inline static std::shared_ptr<surface_renderer> create(const std::shared_ptr<surface> &surface, vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment) {
            return std::make_shared<surface_renderer>(context->shared_from_this(), surface, usage);
        }

        void render_with(const std::shared_ptr<renderer_base> &renderer);
        void create_image_views();

        static constexpr uint32_t max_frames_in_flight = 2;

      private:
        std::shared_ptr<vulkan_context> _context;
        std::shared_ptr<swapchain>      _swapchain;

        std::vector<std::unique_ptr<sync_resources>> _syncs;
        vk::raii::CommandPool                        _command_pool;
        std::vector<vk::raii::CommandBuffer>         _command_buffers;
        uint32_t                                     _current_frame = 0;

        std::vector<vk::raii::ImageView> _image_views;
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

    template <class T>
    concept renderer_mixin = requires(T *mixin, renderer_base* renderer, const frame_resources &frame_resources) {
        T();
        mixin->pre_render_frame(frame_resources, renderer);
        mixin->post_render_frame(frame_resources, renderer);
    };

    template <renderer_mixin A>
    std::tuple<A*> construct_mixins() {
        return std::make_tuple(new A());
    }

    template <renderer_mixin A, renderer_mixin B, renderer_mixin... Mixins>
    std::tuple<A *, B*, Mixins *...> construct_mixins() {
        return std::tuple_cat<std::tuple<A *>, std::tuple<B*, Mixins *...>>(std::make_tuple<A *>(new A()), construct_mixins<B, Mixins...>());
    }

    template<std::size_t i, renderer_mixin... Mixins>
    void call_prerf(std::tuple<Mixins*...> &t, const frame_resources &frame_resources, renderer_base* renderer) {
        std::get<i>(t)->pre_render_frame(frame_resources, renderer);
        if constexpr (i + 1 < sizeof...(Mixins)) {
            call_prerf<i + 1, Mixins...>(t, frame_resources, renderer);
        }
    }

    template<std::size_t i, renderer_mixin... Mixins>
    void call_postrf(std::tuple<Mixins*...> &t, const frame_resources &frame_resources, renderer_base* renderer) {
        if constexpr (i + 1 < sizeof...(Mixins)) {
            call_postrf<i + 1, Mixins...>(t, frame_resources, renderer);
        }
        std::get<i>(t)->pre_render_frame(frame_resources, renderer);
    }

    template<std::size_t i, renderer_mixin... Mixins>
    void del_mixins(std::tuple<Mixins*...> &t) {
        delete std::get<i>(t);
        if constexpr (i + 1 < sizeof...(Mixins)) {
            del_mixins<i + 1, Mixins...>(t);
        }
    }


    template <renderer_mixin... Mixins>
    class renderer : public renderer_base {
      public:
        renderer() : _mixins(construct_mixins<Mixins...>()) {}
        renderer(const std::tuple<Mixins *...> &mixins) : _mixins(mixins) {}

        ~renderer() override {
            del_mixins<0, Mixins...>(_mixins);
        }

        void pre_render_frame(const frame_resources &frame_resources) override {
            call_prerf<0, Mixins...>(_mixins, frame_resources, this);
        }

        void post_render_frame(const frame_resources &frame_resources) override {
            call_postrf<0, Mixins...>(_mixins, frame_resources, this);
        }

        template <std::size_t I>
        auto get_mixin() const {
            return std::get<I>(_mixins);
        }

      private:
        std::tuple<Mixins *...> _mixins;
    };

    class dynamic_rendering {
      public:
        dynamic_rendering() = default;
        void pre_render_frame(const frame_resources &frame_resources, const renderer_base * renderer);
        void post_render_frame(const frame_resources &frame_resources, const renderer_base* renderer);

        vk::AttachmentLoadOp  load_op  = vk::AttachmentLoadOp::eClear;
        vk::AttachmentStoreOp store_op = vk::AttachmentStoreOp::eStore;
        vk::ClearColorValue   clear_color{0.0f, 0.0f, 0.0f, 1.0f};
    };
} // namespace neuron::render
