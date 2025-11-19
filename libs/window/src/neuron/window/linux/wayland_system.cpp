//
// Created by andy on 11/17/25.
//
#if defined(NEURON_WINDOW_TARGET_LINUX) && defined(NEURON_WINDOW_WAYLAND_SUPPORT)

#define VK_USE_PLATFORM_WAYLAND_KHR

#include "wayland_system.hpp"

#include <cerrno>
#include <cstring>
#include <ctime>

#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>
#include <unistd.h>


static void randname(char *buf) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long r = ts.tv_nsec;
    for (int i = 0; i < 6; ++i) {
        buf[i] = 'A' + (r & 15) + (r & 16) * 2;
        r >>= 5;
    }
}

static int create_shm_file(void) {
    int retries = 100;
    do {
        char name[] = "/wl_shm-XXXXXX";
        randname(name + sizeof(name) - 7);
        --retries;
        int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            shm_unlink(name);
            return fd;
        }
    } while (retries > 0 && errno == EEXIST);
    return -1;
}

static int allocate_shm_file(const std::size_t size) {
    int fd = create_shm_file();
    if (fd < 0)
        return -1;
    int ret;
    do {
        ret = ftruncate(fd, size);
    } while (ret < 0 && errno == EINTR);
    if (ret < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

namespace neuron::window {
    wayland_system::wayland_system() {
        _display = wl_display_connect(nullptr);
        if (_display == nullptr) {
            throw std::runtime_error("Failed to connect to Wayland display.");
        }

        _registry = wl_display_get_registry(_display);

        _registry_listener.global = +[](void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
            if (!data) {
                return;
            }
            // bind globals
            auto *wl = static_cast<wayland_system *>(data);
            if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
                wl->_compositor = static_cast<wl_compositor *>(wl_registry_bind(registry, name, &wl_compositor_interface, 4));
            } else if (std::strcmp(interface, wl_shm_interface.name) == 0) {
                wl->_shm = static_cast<wl_shm *>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
            } else if (std::strcmp(interface, xdg_wm_base_interface.name) == 0) {
                wl->_xdg_wm_base = static_cast<::xdg_wm_base *>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
                xdg_wm_base_add_listener(wl->_xdg_wm_base, &wl->_xdg_wm_base_listener, wl);
            } else if (std::strcmp(interface, wl_output_interface.name) == 0) {
                wl->_output = static_cast<wl_output *>(wl_registry_bind(registry, name, &wl_output_interface, 2));
                wl_output_add_listener(wl->_output, &wl->_output_listener, wl);
            } else if (std::strcmp(interface, wl_seat_interface.name) == 0) {
                wl->_seat = static_cast<wl_seat *>(wl_registry_bind(registry, name, &wl_seat_interface, 7));
                wl_seat_add_listener(wl->_seat, &wl->_seat_listener, wl);
            } else if (std::strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
                wl->_decoration_manager = static_cast<zxdg_decoration_manager_v1 *>(wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, 1));
            }
        };
        _registry_listener.global_remove = +[](void *data, wl_registry *registry, uint32_t name) {};

        _xdg_wm_base_listener.ping = +[](void *data, ::xdg_wm_base *xdg_wm_base, uint32_t serial) { xdg_wm_base_pong(xdg_wm_base, serial); };

        _output_listener.scale = +[](void *data, wl_output *output, int factor) {
            auto *wl          = static_cast<wayland_system *>(data);
            wl->_scale_factor = factor;
        };
        _output_listener.geometry = +[](void *data, wl_output *output, int x, int y, int w, int h, int subpixel, const char *make, const char *model, int transform) {
            auto *wl             = static_cast<wayland_system *>(data);
            wl->_output_geometry = wayland_output_geometry{.x = x, .y = y, .width = w, .height = h, .subpixel = subpixel, .make = make, .model = model, .transform = transform};
        };

        _output_listener.mode = +[](void *data, wl_output *output, uint32_t flags, int width, int height, int refresh) {
            if (flags & WL_OUTPUT_MODE_CURRENT) {
                auto *wl         = static_cast<wayland_system *>(data);
                wl->_output_mode = {.flags = flags, .width = width, .height = height, .refresh = refresh};
            }
        };

        _output_listener.name = +[](void *data, wl_output *output, const char *name) {
            auto *wl         = static_cast<wayland_system *>(data);
            wl->_output_name = name;
        };

        _output_listener.description = +[](void *data, wl_output *output, const char *description) {
            auto *wl                = static_cast<wayland_system *>(data);
            wl->_output_description = description;
        };

        _output_listener.done = +[](void *data, wl_output *output) {};

        _seat_listener.name = +[](void *data, wl_seat *seat, const char *name) {};

        _seat_listener.capabilities = +[](void *data, wl_seat *seat, uint32_t capabilities) {};

        wl_registry_add_listener(_registry, &_registry_listener, this);
        wl_display_roundtrip(_display);

        _libdecor_iface = {
            .error = +[](::libdecor *context, libdecor_error error, const char *message) { fprintf(stderr, "[libdecor] (%d) %s\n", error, message); },
        };

        _libdecor = libdecor_new(_display, &_libdecor_iface);
    }
    wayland_system::~wayland_system() {
        wl_display_disconnect(_display);
    }

    std::shared_ptr<shm_pool> wayland_system::create_shm_pool(const std::size_t size) {
        const auto file = std::make_shared<shm_file>(size);
        return create_shm_pool(file);
    }

    std::shared_ptr<shm_pool> wayland_system::create_shm_pool(const std::shared_ptr<shm_file> &shm_file) {
        return std::make_shared<shm_pool>(shm_file, std::static_pointer_cast<wayland_system>(shared_from_this()));
    }

    const std::vector<const char *> &wayland_system::required_instance_extensions() const {
        static std::vector<const char *> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
        };
        return extensions;
    }

    std::shared_ptr<wayland_surface> wayland_system::create_surface() {
        auto *surface = wl_compositor_create_surface(_compositor);
        return std::make_shared<wayland_surface>(std::static_pointer_cast<wayland_system>(shared_from_this()), surface);
    }

    int wayland_system::dispatch() const {
        return wl_display_dispatch(_display);
    }

    // basically just what is in https://github.com/glfw/glfw/blob/master/src/wl_window.c, adapted to my needs
    static bool flush_display(wl_display *display) {
        while (wl_display_flush(display)) {
            if (errno != EAGAIN) {
            }

            pollfd fd = {wl_display_get_fd(display), POLLOUT};
            while (poll(&fd, 1, -1) == -1) {
                if (errno != EINTR && errno != EAGAIN) {
                    return false;
                }
            }
        }
        return true;
    }

    void wayland_system::poll() {

        pollfd fd = {wl_display_get_fd(_display), POLLIN};

        bool event = false;
        while (!event) {
            while (wl_display_prepare_read(_display) != 0) {
                if (wl_display_dispatch_pending(_display) > 0) {
                    return;
                }
            }

            if (!flush_display(_display)) {
                wl_display_cancel_read(_display);

                // whoops looks like we lost wayland
                return;
            }

            const int res = ::poll(&fd, 1, 0) > 0;
            if (res == -1 && errno != EINTR && errno != EAGAIN) {
                wl_display_cancel_read(_display);

                // whoops looks like we lost wayland?
                return;
            }

            if (fd.revents & POLLIN) {
                wl_display_read_events(_display);
                if (wl_display_dispatch_pending(_display) > 0) {
                    event = true;
                }
            } else {
                wl_display_cancel_read(_display);
            }
        }
    }
    std::shared_ptr<window> wayland_system::create_window(const int width, const int height, const std::string_view title) {
        return std::make_shared<wayland_window>(create_surface()->decorate(width, height, title), std::static_pointer_cast<wayland_system>(shared_from_this()));
    }

    wayland_surface::wayland_surface(std::shared_ptr<wayland_system> system, wl_surface *surface) : _system(std::move(system)), _surface(surface) {}

    wayland_surface::~wayland_surface() {
        wl_surface_destroy(_surface);
    }

    void wayland_surface::commit() const {
        wl_surface_commit(_surface);
    }

    std::shared_ptr<decorated_surface> wayland_surface::decorate(const int width, const int height) {
        return std::make_shared<decorated_surface>(shared_from_this(), width, height);
    }

    std::shared_ptr<decorated_surface> wayland_surface::decorate(const int width, const int height, const std::string_view title) {
        auto decor = decorate(width, height);
        decor->set_title(title);
        decor->set_appid(title);
        return decor;
    }

    shm_file::shm_file(const std::size_t size) : _size(size) {
        _fd = allocate_shm_file(_size);
        if (_fd < 0) {
            throw std::runtime_error("Failed to allocate shm file.");
        }
    }

    shm_file::~shm_file() {
        close(_fd);
    }

    void *shm_file::mmap(void *addr, const int prot, const int flags) const {
        return ::mmap(addr, _size, prot, flags, _fd, 0);
    }

    shm_pool::shm_pool(const std::shared_ptr<shm_file> &shm_file, const std::shared_ptr<wayland_system> &system) : _system(system), _shm_file(shm_file) {
        _pool_data = _shm_file->mmap(nullptr, PROT_READ | PROT_WRITE, MAP_SHARED);
        _shm_pool  = wl_shm_create_pool(_system->shm(), _shm_file->fd(), static_cast<int>(_shm_file->size()));
    }

    shm_pool::~shm_pool() {
        munmap(_pool_data, _shm_file->size());
        if (_shm_pool) {
            wl_shm_pool_destroy(_shm_pool);
        }
    }

    shm_buffer::shm_buffer(const std::shared_ptr<shm_pool> &pool, const int offset, const int width, const int height, const int stride, const wl_shm_format format)
        : _shm_pool(pool) {
        _buffer = wl_shm_pool_create_buffer(_shm_pool->_shm_pool, offset, width, height, stride, format);
    }

    shm_buffer::~shm_buffer() {
        wl_buffer_destroy(_buffer);
    }

    decorated_surface::decorated_surface(const std::shared_ptr<wayland_surface> &surface, int width, int height)
        : _surface(surface), _default_width(width), _default_height(height) {
        _libdecor_frame_iface = {
            .configure =
                +[](libdecor_frame *frame, libdecor_configuration *configuration, void *user_data) {
                    auto *s = static_cast<decorated_surface *>(user_data);
                    if (!libdecor_configuration_get_window_state(configuration, &s->_window_state)) {
                        s->_window_state = LIBDECOR_WINDOW_STATE_NONE;
                    }

                    int width, height;
                    if (!libdecor_configuration_get_content_size(configuration, frame, &width, &height)) {
                        width  = s->_default_width;
                        height = s->_default_height;
                    }

                    width  = width == 0 ? s->_default_width : width;
                    height = height == 0 ? s->_default_height : height;

                    s->_width  = width;
                    s->_height = height;


                    s->_state = libdecor_state_new(s->_width, s->_height);
                    libdecor_frame_commit(s->_frame, s->_state, configuration);
                    libdecor_state_free(s->_state);

                    if (libdecor_frame_is_floating(s->_frame)) {
                        s->_default_width  = width;
                        s->_default_height = height;
                    }
                },
            .close =
                +[](libdecor_frame *frame, void *user_data) {
                    auto *s         = static_cast<decorated_surface *>(user_data);
                    s->_wants_close = true;
                },
            .commit        = +[](libdecor_frame *frame, void *user_data) {},
            .dismiss_popup = +[](libdecor_frame *frame, const char *seat_name, void *user_data) {},
        };
        _frame = libdecor_decorate(_surface->_system->libdecor(), _surface->surface(), &_libdecor_frame_iface, this);

        set_title("Window");

        if (_frame) {
            libdecor_frame_map(_frame);
        }
    }

    decorated_surface::~decorated_surface() {}

    void decorated_surface::set_title(const std::string_view title) const {
        libdecor_frame_set_title(_frame, title.data());
    }

    void decorated_surface::set_appid(const std::string_view appid) const {
        libdecor_frame_set_app_id(_frame, appid.data());
    }

    wayland_pointer::wayland_pointer(const std::shared_ptr<wayland_system> &system) : _system(system) {
        _pointer                = wl_seat_get_pointer(system->seat());
        _pointer_listener.enter = +[](void *data, wl_pointer *pointer, uint32_t serial, wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {
            auto *p = static_cast<wayland_pointer *>(data);
            p->_enter(serial, surface, surface_x, surface_y);
        };
        _pointer_listener.leave = +[](void *data, wl_pointer *pointer, uint32_t serial, wl_surface *surface) {
            auto *p = static_cast<wayland_pointer *>(data);
            p->_leave(serial, surface);
        };
        _pointer_listener.motion = +[](void *data, wl_pointer *pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
            auto *p = static_cast<wayland_pointer *>(data);
            p->_motion(time, surface_x, surface_y);
        };
        _pointer_listener.button = +[](void *data, wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
            auto *p = static_cast<wayland_pointer *>(data);
            p->_button(serial, time, button, state);
        };
        _pointer_listener.axis = +[](void *data, wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {
            auto *p = static_cast<wayland_pointer *>(data);
            p->_axis(time, axis, value);
        };
        _pointer_listener.frame = +[](void *data, wl_pointer *pointer) {
            auto *p = static_cast<wayland_pointer *>(data);
            p->_frame();
        };
        _pointer_listener.axis_source = +[](void *data, wl_pointer *pointer, uint32_t axis_source) {
            auto *p = static_cast<wayland_pointer *>(data);
            p->_axis_source(axis_source);
        };
        _pointer_listener.axis_stop = +[](void *data, wl_pointer *pointer, uint32_t time, uint32_t axis) {
            auto *p = static_cast<wayland_pointer *>(data);
            p->_axis_stop(time, axis);
        };
        _pointer_listener.axis_discrete = +[](void *data, wl_pointer *pointer, uint32_t axis, int32_t discrete) {
            auto *p = static_cast<wayland_pointer *>(data);
            p->_axis_discrete(axis, discrete);
        };
        _pointer_listener.axis_value120 = +[](void *data, wl_pointer *pointer, uint32_t axis, int32_t value_120) {
            auto *p = static_cast<wayland_pointer *>(data);
            p->_axis_value_120(axis, value_120);
        };
        _pointer_listener.axis_relative_direction = +[](void *data, wl_pointer *pointer, uint32_t axis, uint32_t direction) {
            auto *p = static_cast<wayland_pointer *>(data);
            p->_axis_relative_direction(axis, direction);
        };

        wl_pointer_add_listener(_pointer, &_pointer_listener, this);
    }

    void wayland_pointer::_enter(uint32_t serial, wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {}
    void wayland_pointer::_leave(uint32_t serial, wl_surface *surface) {}
    void wayland_pointer::_motion(uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y) {}
    void wayland_pointer::_button(uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {}
    void wayland_pointer::_axis(uint32_t time, uint32_t axis, wl_fixed_t value) {}
    void wayland_pointer::_frame() {}
    void wayland_pointer::_axis_source(uint32_t axis_source) {}
    void wayland_pointer::_axis_stop(uint32_t time, uint32_t axis) {}
    void wayland_pointer::_axis_discrete(uint32_t axis, int32_t discrete) {}
    void wayland_pointer::_axis_value_120(uint32_t axis, int32_t value_120) {}
    void wayland_pointer::_axis_relative_direction(uint32_t axis, uint32_t direction) {}

    wayland_window::wayland_window(const std::shared_ptr<decorated_surface> &surface, const std::shared_ptr<wayland_system> &system) : window(system), _surface(surface) {
        wl_display_roundtrip(system->display());
        wl_display_roundtrip(system->display());
    }

    vk::raii::SurfaceKHR wayland_window::create_surface(const vk::raii::Instance &instance) {
        VkWaylandSurfaceCreateInfoKHR surface_create_info{};
        surface_create_info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        surface_create_info.display = std::static_pointer_cast<wayland_system>(_system)->display();
        surface_create_info.surface = _surface->wlsurface()->surface();
        auto proc = reinterpret_cast<PFN_vkCreateWaylandSurfaceKHR>(instance.getProcAddr("vkCreateWaylandSurfaceKHR"));
        VkSurfaceKHR surf;
        if (proc(*instance, &surface_create_info, nullptr, &surf) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create surface!");
        }

        return vk::raii::SurfaceKHR(instance, surf);
    }
} // namespace neuron::window
#endif