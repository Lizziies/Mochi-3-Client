#include "Ui.hpp"
#include "Fonts.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"
#include "gui/Gui.hpp"
#include "gui/Notify.hpp"
#include "gui/Theme.hpp"
#include "modules/Manager.hpp"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <algorithm>
#include <atomic>
#include <string>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace ui {

static bool ready = false;
static std::atomic<bool> capture{false};
static std::atomic<bool> cursor{false};
static float uiScale = 1.f;
static float appliedScale = 0.f;
static std::string iniPath;

bool init(HWND window, ID3D11Device* device, ID3D11DeviceContext* context) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    iniPath = logger::narrow((paths::root() / L"imgui.ini").wstring());
    io.IniFilename = iniPath.c_str();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    fonts::load();
    theme::applyStyle();

    if (!ImGui_ImplWin32_Init(window) || !ImGui_ImplDX11_Init(device, context)) {
        logger::error("imgui backend init failed");
        return false;
    }
    ready = true;
    logger::info("ui ready");
    return true;
}

static void updateScale() {
    auto& io = ImGui::GetIO();
    if (io.DisplaySize.y > 0) uiScale = std::clamp(io.DisplaySize.y / 1080.f, 0.7f, 2.5f);
    if (uiScale == appliedScale) return;
    appliedScale = uiScale;
    theme::applyStyle();
}

void frame() {
    if (!ready) return;
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    updateScale();
    ImGui::NewFrame();

    gui::beginFrame();
    modules::frame(ImGui::GetBackgroundDrawList());
    gui::draw();
    notify::draw();

    capture = gui::wantsInput();
    cursor = gui::wantsCursor();
    ImGui::GetIO().MouseDrawCursor = cursor;

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void shutdown() {
    if (!ready) return;
    ready = false;
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void invalidate() {}

bool wndProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    if (!ready) return false;
    ImGui_ImplWin32_WndProcHandler(w, msg, wp, lp);
    return capture;
}

bool wantsCursor() { return cursor; }
float scale() { return uiScale; }
float dt() { return ready ? ImGui::GetIO().DeltaTime : 0.016f; }
double time() { return ready ? ImGui::GetTime() : 0.0; }

}
