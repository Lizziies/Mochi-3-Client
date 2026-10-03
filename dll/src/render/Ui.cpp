#include "Ui.hpp"
#include "Fonts.hpp"
#include "core/Config.hpp"
#include "core/Log.hpp"
#include "gui/Gui.hpp"
#include "gui/Notify.hpp"
#include "gui/Profile.hpp"
#include "gui/Theme.hpp"
#include "hook/Input.hpp"
#include "modules/Manager.hpp"
#include "core/Client.hpp"
#include "core/FlarialLink.hpp"
#include "hook/Dx.hpp"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <algorithm>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace ui {

static bool ready = false;
static std::atomic<bool> capture{false};
static std::atomic<bool> cursor{false};
static float uiScale = 1.f;
static float appliedScale = 0.f;

static ID3D11Device* device11 = nullptr;
static ID3D11DeviceContext* context11 = nullptr;

bool init(HWND window, ID3D11Device* device, ID3D11DeviceContext* context) {
    device11 = device;
    context11 = context;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
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

// Window messages arrive on the game's window thread, ImGui runs on the render thread. Feeding ImGui from the
// window thread races NewFrame, so everything waits here and is handed over at the start of the next frame.
namespace {

struct Pending {
    HWND w;
    UINT msg;
    WPARAM wp;
    LPARAM lp;
    int button;
    float wheel;
};

std::mutex inboxLock;
std::vector<Pending> inbox, draining;

void post(const Pending& p) {
    std::scoped_lock g(inboxLock);
    if (inbox.size() < 4096) inbox.push_back(p);
}

void drain() {
    {
        std::scoped_lock g(inboxLock);
        draining.swap(inbox);
    }
    auto& io = ImGui::GetIO();
    for (auto& p : draining) {
        if (p.button >= 0) io.AddMouseButtonEvent(p.button, p.msg != 0);
        else if (p.wheel != 0.f) io.AddMouseWheelEvent(0.f, p.wheel);
        else ImGui_ImplWin32_WndProcHandler(p.w, p.msg, p.wp, p.lp);
    }
    draining.clear();
}

}

void frame() {
    if (!ready) return;
    input::Ours ours;
    double began = gui::profile::stamp();
    drain();
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    updateScale();
    ImGui::NewFrame();

    gui::beginFrame();
    modules::frame(ImGui::GetBackgroundDrawList());
    flarialLink::frame(device11, context11, dx::swapchain());
    if (flarialLink::ejectRequested()) client::requestUnload();
    gui::draw();
    notify::draw();
    config::tick();

    capture = gui::wantsInput();
    cursor = gui::wantsCursor();
    ImGui::GetIO().MouseDrawCursor = cursor;
    input::syncCursor(cursor);

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    gui::profile::frame(float(gui::profile::since(began)));
    gui::profile::finish(ImGui::GetIO().DeltaTime);
}

void shutdown() {
    if (!flarialLink::stop()) {
        logger::warn("ui stays up for the flarial core");
        return;
    }
    if (!ready) return;
    ready = false;
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void invalidate() {}

bool wndProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    if (!ready) return false;
    bool menu = capturing();
    // with the menu closed ImGui needs no mouse movement, and it never reads WM_INPUT
    if (msg == WM_INPUT || (!menu && msg == WM_MOUSEMOVE)) return menu;
    post({w, msg, wp, lp, -1, 0.f});
    return menu;
}

void mouseButton(int button, bool down) {
    if (ready) post({nullptr, down ? 1u : 0u, 0, 0, button, 0.f});
}

void mouseWheel(float delta) {
    if (ready) post({nullptr, 0, 0, 0, -1, delta});
}

bool wantsCursor() { return cursor || gui::wantsCursor(); }
bool capturing() { return ready && gui::wantsInput(); }
float scale() { return uiScale; }
float dt() { return ready ? ImGui::GetIO().DeltaTime : 0.016f; }
double time() { return ready ? ImGui::GetTime() : 0.0; }

}
