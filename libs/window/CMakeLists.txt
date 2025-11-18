cmake_minimum_required(VERSION 3.30)
project(neuron_window)

find_package(Vulkan REQUIRED)

add_library(neuron_window src/neuron/window/window.hpp
        src/neuron/window/linux/linux.hpp
        src/neuron/window/interface.hpp
        src/neuron/window/linux/x11_system.hpp
        src/neuron/window/linux/x11_system.cpp
        src/neuron/window/linux/wayland_system.hpp
        src/neuron/window/linux/wayland_system.cpp
        src/neuron/window/linux/linux.cpp)
add_library(neuron::window ALIAS neuron_window)

target_include_directories(neuron_window PUBLIC src/)
target_link_libraries(neuron_window PUBLIC neuron_core Vulkan::Headers)

if (LINUX)
    find_library(X11_LIB NAMES X11)
    find_library(WAYLAND_CLIENT_LIB NAMES wayland-client)

    if (X11_LIB)
        set(NEURON_WINDOW_X11_SUPPORT_DEFAULT ON)
    endif ()
    if (WAYLAND_CLIENT_LIB)
        set(NEURON_WINDOW_WAYLAND_SUPPORT_DEFAULT ON)
    endif ()
else ()
    set(NEURON_WINDOW_WAYLAND_SUPPORT_DEFAULT OFF)
    set(NEURON_WINDOW_X11_SUPPORT_DEFAULT OFF)
endif ()

option(NEURON_WINDOW_WAYLAND_SUPPORT "" ${NEURON_WINDOW_WAYLAND_SUPPORT_DEFAULT})
option(NEURON_WINDOW_X11_SUPPORT "" ${NEURON_WINDOW_X11_SUPPORT_DEFAULT})

if (LINUX)
    if (NEURON_WINDOW_X11_SUPPORT)
        target_link_libraries(neuron_window PUBLIC ${X11_LIB})
        target_compile_definitions(neuron_window PUBLIC NEURON_WINDOW_X11_SUPPORT)
        message(STATUS "Including support for X11 in neuron::window")
    endif ()

    if (NEURON_WINDOW_WAYLAND_SUPPORT)
        target_link_libraries(neuron_window PUBLIC ${WAYLAND_CLIENT_LIB})
        target_compile_definitions(neuron_window PUBLIC NEURON_WINDOW_WAYLAND_SUPPORT)
        message(STATUS "Including support for Wayland in neuron::window")

        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c
                COMMAND sh -c "wayland-scanner private-code < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > xdg-shell-protocol.c"
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                VERBATIM)

        add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/neuron/window/linux/xdg-shell-client-protocol.h
                COMMAND sh -c "wayland-scanner client-header < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > xdg-shell-client-protocol.h"
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/neuron/window/linux/
                VERBATIM)

        target_sources(neuron_window
                PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c
                PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/neuron/window/linux/xdg-shell-client-protocol.h)
    endif ()

    target_compile_definitions(neuron_window PUBLIC NEURON_WINDOW_TARGET_LINUX)
elseif (WIN32)
    target_link_libraries(neuron_window PUBLIC Kernel32 User32 Dwmapi)
    target_compile_definitions(neuron_window PUBLIC NEURON_WINDOW_WIN32_SUPPORT)
    target_compile_definitions(neuron_window PUBLIC NEURON_WINDOW_TARGET_WINDOWS)
    message(STATUS "Including support for Win32 in neuron::window")
endif ()


