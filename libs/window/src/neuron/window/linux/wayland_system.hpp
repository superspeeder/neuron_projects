//
// Created by andy on 11/17/25.
//

#pragma once
#if defined(NEURON_WINDOW_TARGET_LINUX) && defined(NEURON_WINDOW_WAYLAND_SUPPORT)
#include "./linux.hpp"
#include "wayland_system.hpp"

#include <wayland-client.h>
#if !__has_include("./xdg-shell-client-protocol.h")
#error "Missing xdg-shell-client-protocol.h header. Make sure to generate it (use wayland-scanner on /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml)."
#endif

#if !__has_include("./xdg-decoration-unstable-v1-client.h")
#error                                                                                                                                                                             \
    "Missing xdg-decoration-unstable-v1-client.h header. Make sure to generate it (use wayland-scanner on /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1-client.xml)."
#endif

#include "./xdg-decoration-unstable-v1-client.h"
#include "./xdg-shell-client-protocol.h"
#include <libdecor-0/libdecor.h>

#define VK_USE_PLATFORM_WAYLAND_KHR

#include <vulkan/vulkan_raii.hpp>

namespace neuron::window {
    class wayland_surface;
    class shm_file;
    class shm_pool;
    class shm_buffer;
    class decorated_surface;

    struct wayland_output_geometry {
        int         x, y;
        int         width, height;
        int         subpixel;
        std::string make, model;
        int         transform;
    };

    struct wayland_output_mode {
        uint32_t flags;
        int      width, height;
        int      refresh;
    };

    class wayland_system : public linux_system {
      public:
        wayland_system();
        ~wayland_system() override;

        [[nodiscard]] wl_compositor              *compositor() const noexcept { return _compositor; }
        [[nodiscard]] wl_shm                     *shm() const noexcept { return _shm; }
        [[nodiscard]] wl_output                  *output() const noexcept { return _output; }
        [[nodiscard]] wl_seat                    *seat() const noexcept { return _seat; }
        [[nodiscard]] zxdg_decoration_manager_v1 *decoration_manager() const noexcept { return _decoration_manager; }


        [[nodiscard]] std::shared_ptr<shm_pool> create_shm_pool(std::size_t size);
        [[nodiscard]] std::shared_ptr<shm_pool> create_shm_pool(const std::shared_ptr<shm_file> &shm_file);

        [[nodiscard]] ::xdg_wm_base *xdg_wm_base() const noexcept { return _xdg_wm_base; }

        const std::vector<const char *> &required_instance_extensions() override;

        [[nodiscard]] std::shared_ptr<wayland_surface> create_surface();
        [[nodiscard]] wl_display                      *display() const noexcept { return _display; };

        int  dispatch() const;
        void poll() override;

        int                            scale_factor() const { return _scale_factor; }
        const wayland_output_geometry &output_geometry() const { return _output_geometry; }
        const wayland_output_mode     &output_mode() const { return _output_mode; }
        const std::string             &output_name() const { return _output_name; }
        const std::string             &output_description() const { return _output_description; }

        [[nodiscard]] ::libdecor *libdecor() const noexcept { return _libdecor; };

      private:
        wl_display  *_display;
        wl_registry *_registry;

        wl_registry_listener _registry_listener;
        xdg_wm_base_listener _xdg_wm_base_listener;
        wl_output_listener   _output_listener;
        wl_seat_listener     _seat_listener;

        wl_compositor              *_compositor;
        wl_shm                     *_shm;
        wl_output                  *_output;
        wl_seat                    *_seat;
        ::xdg_wm_base              *_xdg_wm_base;
        zxdg_decoration_manager_v1 *_decoration_manager;

        libdecor_interface _libdecor_iface;
        ::libdecor        *_libdecor;

        int                     _scale_factor = 1;
        wayland_output_geometry _output_geometry;
        wayland_output_mode     _output_mode;
        std::string             _output_name;
        std::string             _output_description;
    };

    class wayland_surface : public std::enable_shared_from_this<wayland_surface> {
      public:
        wayland_surface(std::shared_ptr<wayland_system> system, wl_surface *surface);
        ~wayland_surface();

        void commit() const;

        [[nodiscard]] std::shared_ptr<decorated_surface> decorate(int width, int height);
        [[nodiscard]] wl_surface                        *surface() const noexcept { return _surface; }

      private:
        std::shared_ptr<wayland_system> _system;
        wl_surface                     *_surface;

        friend class decorated_surface;
    };

    class shm_file {
      public:
        explicit shm_file(std::size_t size);
        ~shm_file();

        shm_file(const shm_file &other)                = delete;
        shm_file(shm_file &&other) noexcept            = delete;
        shm_file &operator=(const shm_file &other)     = delete;
        shm_file &operator=(shm_file &&other) noexcept = delete;

        [[nodiscard]] std::size_t size() const noexcept { return _size; }
        [[nodiscard]] int         fd() const noexcept { return _fd; }

        [[nodiscard]] void *mmap(void *addr, int prot, int flags) const;

      private:
        int         _fd;
        std::size_t _size;
    };

    class shm_pool : public std::enable_shared_from_this<shm_pool> {
      public:
        shm_pool(const std::shared_ptr<shm_file> &shm_file, const std::shared_ptr<wayland_system> &system);
        ~shm_pool();

        shm_pool(const shm_pool &other)                = delete;
        shm_pool(shm_pool &&other) noexcept            = delete;
        shm_pool &operator=(const shm_pool &other)     = delete;
        shm_pool &operator=(shm_pool &&other) noexcept = delete;

      private:
        std::shared_ptr<wayland_system> _system;
        std::shared_ptr<shm_file>       _shm_file;
        void                           *_pool_data;
        wl_shm_pool                    *_shm_pool{nullptr};

        friend class shm_buffer;
    };

    class shm_buffer {
      public:
        shm_buffer(const std::shared_ptr<shm_pool> &pool, int offset, int width, int height, int stride, wl_shm_format format);
        ~shm_buffer();

      private:
        std::shared_ptr<shm_pool> _shm_pool;
        wl_buffer                *_buffer = nullptr;
    };

    class decorated_surface {
      public:
        decorated_surface(const std::shared_ptr<wayland_surface> &surface, int width, int height);
        ~decorated_surface();

        void set_title(std::string_view title) const;
        void set_appid(std::string_view appid) const;

        [[nodiscard]] inline int width() const noexcept { return _width; }
        [[nodiscard]] inline int height() const noexcept { return _height; }

        [[nodiscard]] inline bool should_close() const noexcept { return _wants_close; }

      private:
        std::shared_ptr<wayland_surface> _surface;

        libdecor_frame_interface _libdecor_frame_iface;
        libdecor_frame          *_frame;
        libdecor_window_state    _window_state;
        libdecor_state          *_state;

        int _default_width, _default_height;
        int _width, _height;
        bool _wants_close = false;
    };

    class wayland_pointer {
      public:
        explicit wayland_pointer(const std::shared_ptr<wayland_system> &system);

      private:
        std::shared_ptr<wayland_system> _system;
        wl_pointer                     *_pointer;
        wl_pointer_listener             _pointer_listener;

        void _enter(uint32_t serial, wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y);
        void _leave(uint32_t serial, wl_surface *surface);
        void _motion(uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y);
        void _button(uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
        void _axis(uint32_t time, uint32_t axis, wl_fixed_t value);
        void _frame();
        void _axis_source(uint32_t axis_source);
        void _axis_stop(uint32_t time, uint32_t axis);
        void _axis_discrete(uint32_t axis, int32_t discrete);
        void _axis_value_120(uint32_t axis, int32_t value_120);
        void _axis_relative_direction(uint32_t axis, uint32_t direction);
    };
} // namespace neuron::window
#endif
