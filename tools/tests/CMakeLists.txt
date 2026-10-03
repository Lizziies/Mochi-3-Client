cmake_minimum_required(VERSION 3.20)
project(MonchiNativeTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()
add_executable(native_bindings native_bindings.cpp
    ../../dll/src/hook/FreeCamera.cpp
    ../../dll/src/hook/OwnNametag.cpp)
target_include_directories(native_bindings PRIVATE ../../dll/src ../../dll/lib/imgui ../../common)
target_compile_definitions(native_bindings PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
if(MSVC)
    target_compile_options(native_bindings PRIVATE /utf-8 /W3)
endif()
add_test(NAME native_bindings COMMAND native_bindings)
