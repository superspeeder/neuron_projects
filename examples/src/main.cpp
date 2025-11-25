//
// Created by andy on 11/15/25.
//

#include <neuron/window/interface.hpp>

#include <neuron/render/render.hpp>

#include <neuron/render/surface_renderer.hpp>
#include <neuron/window/window.hpp>

#include <chrono>
#include <vulkan/vulkan_raii.hpp>


template <class... Ts>
struct overloads : Ts... {
    using Ts::operator()...;
};

class test_renderer : public neuron::render::renderer_base {
  public:
    test_renderer() : renderer_base() {
        render_layout = vk::ImageLayout::eTransferDstOptimal;
        render_access = vk::AccessFlagBits2::eTransferWrite;
        render_stage  = vk::PipelineStageFlagBits2::eTransfer;
    };

    ~test_renderer() override = default;

    void render_frame(const neuron::render::frame_resources &frame_resources) override {
        const auto &cmd = frame_resources.cmd;
        cmd.clearColorImage(frame_resources.image,
                            render_layout,
                            vk::ClearColorValue(0.0f, 1.0f, 1.0f, 1.0f),
                            {vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)});
    }
};

struct testing_app_state {
    std::shared_ptr<neuron::window::system>           winsys;
    std::shared_ptr<neuron::render::vulkan_context>   vulkan_context;

    std::shared_ptr<neuron::window::window>           window;
    std::shared_ptr<neuron::render::surface_renderer> surface_renderer;
    std::shared_ptr<test_renderer>                    renderer;

    bool running = true;

    testing_app_state() {
        winsys           = neuron::window::init_system();
        vulkan_context   = neuron::render::init_context();

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
