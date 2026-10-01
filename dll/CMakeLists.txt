cmake_minimum_required(VERSION 3.20)
project(Mochi LANGUAGES C CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB MINHOOK_SOURCES lib/minhook/src/*.c lib/minhook/src/hde/hde64.c)

set(IMGUI_SOURCES
    lib/imgui/imgui.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/backends/imgui_impl_dx11.cpp
    lib/imgui/backends/imgui_impl_win32.cpp
)

add_library(Mochi SHARED ${SOURCES} ${MINHOOK_SOURCES} ${IMGUI_SOURCES} res/resources.rc)

target_include_directories(Mochi PRIVATE
    src
    lib/minhook/include
    lib/imgui
    lib/imgui/backends
    lib/json
)

target_compile_definitions(Mochi PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
    IMGUI_DEFINE_MATH_OPERATORS
)

target_link_libraries(Mochi PRIVATE d3d11 d3d12 dxgi d3dcompiler ws2_32 winhttp version psapi winmm dwmapi)

if(MSVC)
    set_property(TARGET Mochi PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(Mochi PRIVATE /utf-8 /W3 /permissive- /EHa)
else()
    target_compile_options(Mochi PRIVATE -Wall -Wno-unknown-pragmas -Wno-missing-field-initializers)
    target_link_options(Mochi PRIVATE -static -static-libgcc -static-libstdc++)
endif()

set_target_properties(Mochi PROPERTIES PREFIX "" OUTPUT_NAME "Mochi")
