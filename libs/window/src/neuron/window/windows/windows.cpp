//
// Created by andy on 11/18/2025.
//

#if defined(NEURON_WINDOW_TARGET_WINDOWS) && defined(NEURON_WINDOW_WIN32_SUPPORT)
#define VK_USE_PLATFORM_WIN32_KHR

#include "windows.hpp"

namespace neuron::window {
    constexpr auto WCNAME = L"Neuron Window Class";

    static std::wstring mb2w(const std::string_view mb) {
        const std::size_t req_chars = MultiByteToWideChar(CP_UTF8, 0, mb.data(), mb.size(), nullptr, 0);

        std::wstring out;
        out.resize(req_chars);
        MultiByteToWideChar(CP_UTF8, 0, mb.data(), mb.size(), out.data(), out.size());
        return out;
    }

    static std::string w2mb(const std::wstring_view w) {
        const std::size_t req_chars = WideCharToMultiByte(CP_UTF8, 0, w.data(), w.size(), nullptr, 0, nullptr, nullptr);

        std::string out;
        out.resize(req_chars);
        WideCharToMultiByte(CP_UTF8, 0, w.data(), w.size(), out.data(), out.size(), nullptr, nullptr);
        return out;
    }

    windows_system::windows_system() {
        _instance = GetModuleHandleW(nullptr);

        WNDCLASSEXW wc{};
        wc.cbSize        = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc   = &windows_window::window_proc;
        wc.lpszClassName = WCNAME;
        wc.style         = CS_VREDRAW | CS_HREDRAW;
        wc.hInstance     = _instance;
        RegisterClassExW(&wc);
    }

    windows_system::~windows_system() {
        UnregisterClassW(WCNAME, _instance);
    }

    const std::vector<const char *> &windows_system::required_extensions(render_interface::instance_extension) const {
        static std::vector<const char *> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        };
        return extensions;
    }

    void windows_system::poll() {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE) > 0) {
            DispatchMessageW(&msg);
            TranslateMessage(&msg);
        }
    }


    std::shared_ptr<window> windows_system::create_window(const int width, const int height, const std::string_view title) {
        return std::make_shared<windows_window>(std::static_pointer_cast<windows_system>(shared_from_this()), width, height, title);
    }
    std::variant<bool, std::monostate> windows_system::get_presentation_support(const vk::raii::Instance &instance, vk::PhysicalDevice physical_device,
                                                                                uint32_t queue_family) const {
        static PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR = nullptr;
        if (vkGetPhysicalDeviceWin32PresentationSupportKHR == nullptr) {
            vkGetPhysicalDeviceWin32PresentationSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR>(
                instance.getProcAddr("vkGetPhysicalDeviceWin32PresentationSupportKHR"));
        }

        return vkGetPhysicalDeviceWin32PresentationSupportKHR(physical_device, queue_family) == vk::True;
    }

    windows_window::windows_window(const std::shared_ptr<windows_system> &system, const int width, const int height, const std::string_view title) : window(system) {
        const auto title_ = mb2w(title);
        _window           = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW,
                                  WCNAME,
                                  title_.c_str(),
                                  WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  width,
                                  height,
                                  nullptr,
                                  nullptr,
                                  system->instance(),
                                  this);
        ShowWindow(_window, SW_NORMAL);
        UpdateWindow(_window);

        RECT r;
        GetClientRect(_window, &r);
        _size = {r.right, r.bottom};
    }

    windows_window::~windows_window() {
        DestroyWindow(_window);
    }

    bool windows_window::should_close() const {
        return _should_close;
    }

    window_size_t windows_window::size() const {
        return _size;
    }

    void windows_window::set_title(std::string_view title) {
        const std::wstring title_ = mb2w(title);
        SetWindowTextW(_window, title_.data());
    }

    vk::raii::SurfaceKHR windows_window::create_surface(const vk::raii::Instance &instance) {
        auto                        func = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(instance.getProcAddr("vkCreateWin32SurfaceKHR"));
        VkWin32SurfaceCreateInfoKHR surface_create_info{};
        surface_create_info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surface_create_info.pNext     = nullptr;
        surface_create_info.flags     = 0;
        surface_create_info.hinstance = std::static_pointer_cast<windows_system>(_system)->instance();
        surface_create_info.hwnd      = _window;
        VkSurfaceKHR surf;
        if (func(*instance, &surface_create_info, nullptr, &surf) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create surface!");
        }
        return vk::raii::SurfaceKHR(instance, surf);
    }
    void windows_window::request_redraw() {
        RedrawWindow(_window, nullptr, nullptr, RDW_INVALIDATE);
    }

    LRESULT windows_window::window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (uMsg == WM_CREATE) {
            auto *cs = reinterpret_cast<CREATESTRUCTW *>(lParam);
            if (cs->lpCreateParams) {
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
                return 0;
            }
        }

        if (const auto l = GetWindowLongPtrW(hwnd, GWLP_USERDATA)) {
            auto *const w = reinterpret_cast<windows_window *>(l);
            return w->_window_proc(hwnd, uMsg, wParam, lParam);
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    LRESULT windows_window::_window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_CLOSE:
            _should_close = true;
            _on_close_request(_should_close);
            break;
        case WM_WINDOWPOSCHANGED: {
            const auto &pos = *reinterpret_cast<WINDOWPOS *>(lParam);
            _pos            = {pos.x, pos.y};
            RECT r;
            GetClientRect(_window, &r);
            _size = {r.right, r.bottom};
        } break;

        case WM_WINDOWPOSCHANGING: {
            const auto &pos = *reinterpret_cast<WINDOWPOS *>(lParam);
            _pos            = {pos.x, pos.y};
            RECT r;
            GetClientRect(_window, &r);
            _size = {r.right, r.bottom};

            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
        } break;
        case WM_PAINT: {
            _on_redraw();
            ValidateRect(_window, nullptr);
        } break;
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }
} // namespace neuron::window

#endif
