cmake_minimum_required(VERSION 3.20)
project(Mochi LANGUAGES C CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(MOCHI_VERSION "0.1.0" CACHE STRING "Client version")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../common/I18n.cpp)
file(GLOB MINHOOK_SOURCES lib/minhook/src/*.c lib/minhook/src/hde/hde64.c)

include(FetchContent)
FetchContent_Declare(lua GIT_REPOSITORY https://github.com/lua/lua.git GIT_TAG v5.4.8 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(lua)
file(GLOB LUA_SOURCES ${lua_SOURCE_DIR}/*.c)
list(FILTER LUA_SOURCES EXCLUDE REGEX "/(lua|luac|onelua|ltests)\\.c$")

option(MOCHI_DEV "Development build with the script explorer, never ship this" OFF)
if(MOCHI_DEV)
    set(CAPSTONE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(CAPSTONE_BUILD_CSTOOL OFF CACHE BOOL "" FORCE)
    set(CAPSTONE_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CAPSTONE_BUILD_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
    set(CAPSTONE_ARCHITECTURE_DEFAULT OFF CACHE BOOL "" FORCE)
    set(CAPSTONE_X86_SUPPORT ON CACHE BOOL "" FORCE)
    FetchContent_Declare(capstone GIT_REPOSITORY https://github.com/capstone-engine/capstone.git GIT_TAG 5.0.3 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(capstone)
endif()

set(IMGUI_SOURCES
    lib/imgui/imgui.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/backends/imgui_impl_dx11.cpp
    lib/imgui/backends/imgui_impl_win32.cpp
)

add_library(Mochi SHARED ${SOURCES} ${MINHOOK_SOURCES} ${LUA_SOURCES} ${IMGUI_SOURCES} res/resources.rc)

target_include_directories(Mochi PRIVATE
    src
    ../common
    lib/minhook/include
    ${lua_SOURCE_DIR}
    lib/imgui
    lib/imgui/backends
    lib/json
    lib/stb
)

target_compile_definitions(Mochi PRIVATE
    MOCHI_VERSION="${MOCHI_VERSION}"
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
    IMGUI_DEFINE_MATH_OPERATORS
)

if(MOCHI_DEV)
    target_compile_definitions(Mochi PRIVATE MOCHI_DEV)
    target_include_directories(Mochi PRIVATE ${capstone_SOURCE_DIR}/include)
    target_link_libraries(Mochi PRIVATE capstone)
endif()

target_link_libraries(Mochi PRIVATE d3d11 d3d12 dxgi d3dcompiler ws2_32 winhttp version psapi winmm dwmapi bcrypt ole32)

if(MSVC)
    set_property(TARGET Mochi PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(Mochi PRIVATE /utf-8 /W3 /permissive- /EHa /bigobj)
else()
    target_compile_options(Mochi PRIVATE -Wall -Wno-unknown-pragmas -Wno-missing-field-initializers)
    target_link_options(Mochi PRIVATE -static -static-libgcc -static-libstdc++)
endif()

set_target_properties(Mochi PROPERTIES PREFIX "" OUTPUT_NAME "Mochi")
