#include "FlarialLink.hpp"
#include "Guard.hpp"
#include "Log.hpp"
#include "Paths.hpp"
#include "flarial/Bridge/Bridge.hpp"

#include <imgui.h>

namespace flarialLink {

namespace {

HMODULE core = nullptr;
MonchiFlarialFrame drawCore = nullptr;
MonchiFlarialStop stopCore = nullptr;
MonchiFlarialEjectRequested ejectCore = nullptr;
bool tried = false;

template <class T>
T get(const char* name) {
    return reinterpret_cast<T>(GetProcAddress(core, name));
}

// The core installs game hooks and adds its fonts to ImGui when it starts, so it starts on the render thread right
// before the first frame that has an ImGui context.
void start() {
    tried = true;
    auto file = paths::dllDir() / L"MonchiFlarial.dll";
    core = LoadLibraryW(file.c_str());
    if (!core) {
        logger::info("flarial core not found, running on monchi's modules only");
        return;
    }
    auto begin = get<MonchiFlarialStart>("monchiFlarialStart");
    drawCore = get<MonchiFlarialFrame>("monchiFlarialFrame");
    stopCore = get<MonchiFlarialStop>("monchiFlarialStop");
    ejectCore = get<MonchiFlarialEjectRequested>("monchiFlarialEjectRequested");
    MonchiFlarialImGui imgui{ImGui::GetCurrentContext()};
    ImGui::GetAllocatorFunctions(&imgui.alloc, &imgui.free, &imgui.user);
    bool started = false;
    if (begin && drawCore && stopCore && ejectCore) guard::call("flarial start", [&] { started = begin(&imgui); });
    if (started) {
        logger::info("flarial core started");
        return;
    }
    logger::warn("flarial core did not start");
    drawCore = nullptr;
    // a start that failed half way may already have hooks in the game; they are taken out before anything else
    stop();
}

}

void frame(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapchain) {
    if (!tried) start();
    if (drawCore) guard::call("flarial frame", [&] { drawCore(device, context, swapchain); });
}

bool stop() {
    if (!core) return true;
    drawCore = nullptr;
    bool clean = false;
    if (stopCore) guard::call("flarial stop", [&] { clean = stopCore(); });
    // only a confirmed clean stop may free the code; otherwise the handle is kept and stop() can be tried again
    if (!clean) {
        logger::warn("flarial core did not stop cleanly, it stays loaded");
        return false;
    }
    FreeLibrary(core);
    core = nullptr;
    return true;
}

bool ejectRequested() { return drawCore && ejectCore && ejectCore(); }

}
