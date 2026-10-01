cmake_minimum_required(VERSION 3.20)
project(MochiLauncher LANGUAGES C CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(LIB ${CMAKE_CURRENT_SOURCE_DIR}/../dll/lib)
set(MOCHI_VERSION "0.1.0" CACHE STRING "Version shown in the launcher")

file(GLOB SOURCES CONFIGURE_DEPENDS src/*.cpp)
list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../common/I18n.cpp)

set(MOCHI_DLL "" CACHE FILEPATH "Mochi.dll to embed, makes the launcher a single exe")
set(MOCHI_COSMETICS "" CACHE PATH "Cosmetics folder to embed next to it")
if(MOCHI_DLL AND EXISTS "${MOCHI_DLL}")
    file(TO_CMAKE_PATH "${MOCHI_DLL}" EMBED_DLL)
    set(EMBED_RC ${CMAKE_CURRENT_BINARY_DIR}/embed.rc)
    set(EMBED_TEXT "#include \"${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h\"\nIDR_CLIENT RCDATA \"${EMBED_DLL}\"\n")
    set(EMBED_DEPENDS "${MOCHI_DLL}")
    find_package(Python3 COMPONENTS Interpreter)
    if(MOCHI_COSMETICS AND EXISTS "${MOCHI_COSMETICS}" AND Python3_FOUND)
        set(EMBED_PACK ${CMAKE_CURRENT_BINARY_DIR}/cosmetics.pack)
        execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tools/pack.py ${EMBED_PACK} ${MOCHI_COSMETICS})
        string(APPEND EMBED_TEXT "IDR_COSMETICS RCDATA \"${EMBED_PACK}\"\n")
        list(APPEND EMBED_DEPENDS ${EMBED_PACK})
    endif()
    file(WRITE ${EMBED_RC} "${EMBED_TEXT}")
    list(APPEND SOURCES ${EMBED_RC})
    set_source_files_properties(${EMBED_RC} PROPERTIES OBJECT_DEPENDS "${EMBED_DEPENDS}")
endif()

add_executable(MochiLauncher WIN32
    ${SOURCES}
    ${LIB}/imgui/imgui.cpp
    ${LIB}/imgui/imgui_draw.cpp
    ${LIB}/imgui/imgui_tables.cpp
    ${LIB}/imgui/imgui_widgets.cpp
    ${LIB}/imgui/backends/imgui_impl_dx11.cpp
    ${LIB}/imgui/backends/imgui_impl_win32.cpp
    res/launcher.rc
)

target_include_directories(MochiLauncher PRIVATE src ${CMAKE_CURRENT_SOURCE_DIR}/../common ${LIB}/imgui ${LIB}/imgui/backends ${LIB}/json)

target_compile_definitions(MochiLauncher PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
    IMGUI_DEFINE_MATH_OPERATORS
    MOCHI_VERSION="${MOCHI_VERSION}"
)

target_link_libraries(MochiLauncher PRIVATE d3d11 dxgi d3dcompiler winhttp version bcrypt dwmapi shell32 ole32 comdlg32 advapi32 uuid)

if(MSVC)
    set_property(TARGET MochiLauncher PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(MochiLauncher PRIVATE /utf-8 /W3 /permissive-)
    target_link_options(MochiLauncher PRIVATE /MANIFEST:NO)
else()
    target_compile_options(MochiLauncher PRIVATE -Wall -Wno-unknown-pragmas -Wno-missing-field-initializers)
    target_link_options(MochiLauncher PRIVATE -static -static-libgcc -static-libstdc++ -municode)
endif()
