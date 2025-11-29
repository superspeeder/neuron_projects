//
// Created by andy on 11/15/25.
//

#include "neuron/clock.hpp"
#include "neuron/events/events.hpp"


#include <neuron/window/interface.hpp>

#include <neuron/render/render.hpp>

#include <neuron/render/surface_renderer.hpp>
#include <neuron/window/window.hpp>

#include <chrono>
#include <iostream>
#include <vulkan/vulkan_raii.hpp>

class test_renderer : public neuron::render::renderer<neuron::render::dynamic_rendering> {
  public:
    test_renderer() {};

    ~test_renderer() override = default;

    void render_frame(const neuron::render::frame_resources &frame_resources) override {}
};

struct testing_app_state {
    std::shared_ptr<neuron::window::system>         winsys;
    std::shared_ptr<neuron::render::vulkan_context> vulkan_context;

    std::shared_ptr<neuron::window::window>           window;
    std::shared_ptr<neuron::render::surface_renderer> surface_renderer;
    std::shared_ptr<test_renderer>                    renderer;

    neuron::clock clock;

    bool running = true;

    testing_app_state() {
        winsys         = neuron::window::init_system();
        vulkan_context = neuron::render::init_context();

        window           = neuron::window::create_window(800, 600, "Neuron Example App");
        surface_renderer = neuron::render::surface_renderer::create(window, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst);

        renderer         = std::make_shared<test_renderer>();

        window->set_on_redraw_callback([&] { update(); });
    }

    void mainloop() {
        while (running && !window->should_close()) {
            winsys->poll();
            window->request_redraw();
        }
    }

    void update() {
        surface_renderer->render_with(renderer);

        clock.tick();

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
