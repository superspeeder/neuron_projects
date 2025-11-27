//
// Created by andy on 11/15/25.
//

#include "neuron/events/events.hpp"


#include <neuron/window/interface.hpp>

#include <neuron/render/render.hpp>

#include <neuron/render/surface_renderer.hpp>
#include <neuron/window/window.hpp>

#include <chrono>
#include <iostream>
#include <vulkan/vulkan_raii.hpp>


template <class... Ts>
struct overloads : Ts... {
    using Ts::operator()...;
};

struct shithead_event {
    int i;
};

class test_renderer : public neuron::render::renderer<neuron::render::dynamic_rendering> {
  public:
    test_renderer() {
        render_layout = vk::ImageLayout::eColorAttachmentOptimal;
        render_access = vk::AccessFlagBits2::eColorAttachmentWrite;
        render_stage  = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        get_mixin<0>()->clear_color = vk::ClearColorValue(1.0f, 0.0f, 0.0f, 1.0f);

    };

    ~test_renderer() override = default;

    void render_frame(const neuron::render::frame_resources &frame_resources) override {

    }
};

struct testing_app_state {
    std::shared_ptr<neuron::window::system>         winsys;
    std::shared_ptr<neuron::render::vulkan_context> vulkan_context;

    std::shared_ptr<neuron::window::window>           window;
    std::shared_ptr<neuron::render::surface_renderer> surface_renderer;
    std::shared_ptr<test_renderer>                    renderer;

    bool running = true;

    testing_app_state() {
        winsys         = neuron::window::init_system();
        vulkan_context = neuron::render::init_context();

        window           = neuron::window::create_window(800, 600, "Neuron Example App");
        surface_renderer = neuron::render::surface_renderer::create(window, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst);
        renderer         = std::make_shared<test_renderer>();

        window->set_on_redraw_callback([&] { update(); });

        neuron::events::add_listener(+[](shithead_event *evt) { std::cout << "shithead: " << evt->i << std::endl; });

        neuron::events::dispatch(shithead_event{4});
        neuron::events::dispatch(shithead_event{6});
    }

    void mainloop() {
        while (running && !window->should_close()) {
            winsys->poll();
            window->request_redraw();
        }
    }

    using clock           = std::chrono::high_resolution_clock;
    using duration        = std::chrono::duration<double>;
    using time_point      = std::chrono::time_point<clock, duration>;
    time_point this_frame = clock::now();
    time_point last_frame = clock::now() - duration(1.0 / 60.0);

    void update() {
        surface_renderer->render_with(renderer);

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
