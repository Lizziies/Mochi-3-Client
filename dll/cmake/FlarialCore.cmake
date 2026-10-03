# Flarial's game side (SDK, hooks, events, module logic) as a static library inside Mochi.
# Its menu, config storage, overlay, discord and scripting are not built; Mochi provides those.
include(FetchContent)

set(FLARIAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/flarial")

# pinned to the commits Flarial's floating tags pointed at on 2026-10-03
FetchContent_Declare(entt GIT_REPOSITORY https://github.com/skypjack/entt.git GIT_TAG 303801c23bf116404fc687bdc103a3a696c17d97)
FetchContent_Declare(nes GIT_REPOSITORY https://github.com/DisabledMallis/NuvolaEventSystem.git GIT_TAG a7b288004925916c2368ea8acee2cfc962c3c6b9)
FetchContent_Declare(libhat GIT_REPOSITORY https://github.com/BasedInc/libhat.git GIT_TAG d6297514b05fd238f5b46d003325f108c22741e3)
FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG 9197f51593dab5f637454a8889fb9638e737711a)
FetchContent_Declare(magic_enum GIT_REPOSITORY https://github.com/Neargye/magic_enum.git GIT_TAG ccb76393f9abecc2cbcd47f8b2d2f6760a3d9a4a)
FetchContent_Declare(safetyhook GIT_REPOSITORY https://github.com/cursey/safetyhook.git GIT_TAG f44cc070a8340f2f26649553c49533475417304d)
FetchContent_Declare(jsoncpp GIT_REPOSITORY https://github.com/EquinoxHouse/jsoncpp.git GIT_TAG 3000d0b3ee1a9c3dbe0c37129f3602442c1ccf97)
set(FMT_MODULE OFF CACHE BOOL "" FORCE)
set(SAFETYHOOK_FETCH_ZYDIS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(entt nes libhat fmt magic_enum safetyhook jsoncpp)

file(GLOB_RECURSE FLARIAL_SOURCES CONFIGURE_DEPENDS
    "${FLARIAL_DIR}/src/SDK/*.cpp"
    "${FLARIAL_DIR}/src/Utils/Memory/*.cpp"
    "${FLARIAL_DIR}/src/Client/Hook/*.cpp"
    "${FLARIAL_DIR}/src/Client/Events/*.cpp")

add_library(flarial_core STATIC ${FLARIAL_SOURCES})
set_target_properties(flarial_core PROPERTIES CXX_STANDARD 23 MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_options(flarial_core PRIVATE /utf-8 /bigobj /permissive- /EHa /W0)
target_compile_definitions(flarial_core PRIVATE FLARIAL_VERSION="mochi" FLARIAL_BUILD_TYPE="Release" FLARIAL_BUILD_DATE="" COMMIT_HASH="40ad187" NOMINMAX)
# Flarial uses Mochi's ImGui and MinHook, so one copy of each lives in the dll (two hook engines or two ImGui
# versions would fight over the same functions and globals)
set(FLARIAL_SHIM "${CMAKE_CURRENT_BINARY_DIR}/flarial-shim")
file(WRITE "${FLARIAL_SHIM}/minhook/MinHook.h" "#pragma once
#include \"${CMAKE_CURRENT_SOURCE_DIR}/lib/minhook/include/MinHook.h\"
")
foreach(backend imgui_impl_dx11 imgui_impl_dx12 imgui_impl_win32)
    file(WRITE "${FLARIAL_SHIM}/imgui/${backend}.h" "#pragma once
#include \"${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/backends/${backend}.h\"
")
endforeach()
file(WRITE "${FLARIAL_SHIM}/imgui/imgui_freetype.h" "#pragma once
#include \"${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/misc/freetype/imgui_freetype.h\"
")
file(WRITE "${FLARIAL_SHIM}/imgui/stb.h" "#pragma once
#include \"${FLARIAL_DIR}/lib/ImGui/stb.h\"
")
target_include_directories(flarial_core BEFORE PUBLIC "${FLARIAL_SHIM}" "${CMAKE_CURRENT_SOURCE_DIR}/lib" "${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui")
target_precompile_headers(flarial_core PRIVATE "${FLARIAL_DIR}/src/PCH.hpp")
target_include_directories(flarial_core PUBLIC
    "${FLARIAL_DIR}"
    "${FLARIAL_DIR}/src"
    "${FLARIAL_DIR}/src/Client"
    "${FLARIAL_DIR}/src/Client/Module"
    "${FLARIAL_DIR}/lib")
target_link_libraries(flarial_core PUBLIC libhat fmt::fmt EnTT::EnTT NES magic_enum safetyhook jsoncpp)
