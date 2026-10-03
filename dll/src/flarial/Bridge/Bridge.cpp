// SPDX-License-Identifier: AGPL-3.0-only
// Monchi's start of the Flarial core; the client settings list is taken from Flarial src/Client/Client.cpp.
#include "Bridge.hpp"

#include "Client.hpp"
#include "Managers/Managers.hpp"
#include "Events/EventManager.hpp"
#include "Events/Render/RenderEvent.hpp"
#include "GUI/D2D.hpp"
#include "GUI/Engine/Engine.hpp"
#include "GUI/Engine/ExpressionFormat.hpp"
#include "Hook/Manager.hpp"
#include "Hook/Hooks/Render/DirectX/DXGI/SwapchainHook.hpp"
#include "Module/Manager.hpp"
#include "Command/CommandManager.hpp"
#include "Utils/PlatformUtils.hpp"
#include "Utils/Utils.hpp"
#include "Utils/VersionUtils.hpp"
#include "Utils/WinrtUtils.hpp"
#include "../Assets/Assets.hpp"

#include <d2d1_3.h>
#include <dxgi.h>
#include <imgui/imgui.h>
#include <minhook/MinHook.h>

#include <atomic>
#include <filesystem>

namespace {

std::atomic<bool> live{false};
winrt::com_ptr<ID2D1Device> d2dDevice;
ID3D11Device* d2dFor = nullptr;

// Flarial reads these client-wide settings everywhere (fonts, rgb speed, watermark, ...); same names and defaults
// as Flarial's Client::initialize so its modules behave the same
void addClientSettings() {
    ADD_SETTING("fontname", std::string("Space Grotesk"));
    ADD_SETTING("mod_fontname", std::string("Space Grotesk"));
    ADD_SETTING("blurintensity", 2.0f);
    ADD_SETTING("killdx", false);
    ADD_SETTING("disable_alias", false);
    ADD_SETTING("vsync", false);
    ADD_SETTING("recreateAtStart", false);
    ADD_SETTING("promotions", true);
    ADD_SETTING("hideHudAndMods", true);
    ADD_SETTING("saveScrollPos", true);
    ADD_SETTING("pageScrollMultiplier", 5.0f);
    ADD_SETTING("snappinglines", true);
    ADD_SETTING("apiusage", true);
    ADD_SETTING("donotwait", true);
    ADD_SETTING("bufferingmode", std::string("Double Buffering"));
    ADD_SETTING("swapeffect", std::string("FLIP_SEQUENTIAL"));
    ADD_SETTING("disableanims", false);
    ADD_SETTING("anonymousApi", false);
    ADD_SETTING("dlassets", true);
    ADD_SETTING("noicons", false);
    ADD_SETTING("noshadows", false);
    ADD_SETTING("watermark", true);
    ADD_SETTING("centreCursor", false);
    ADD_SETTING("aliasingMode", std::string("Default"));
    ADD_SETTING("ejectKeybind", std::string(""));
    ADD_SETTING("enabledModulesOnTop", true);
    ADD_SETTING("rgb_speed", 1.0f);
    ADD_SETTING("rgb_saturation", 1.0f);
    ADD_SETTING("rgb_value", 1.0f);
    ADD_SETTING("pixelateFonts", false);
    ADD_SETTING("modules_font_scale", 1.0f);
    ADD_SETTING("gui_font_scale", 1.0f);
    ADD_SETTING("overrideFontWeight", false);
    ADD_SETTING("fontWeight", std::string("Normal"));
    ADD_SETTING("nologoicon", false);
    ADD_SETTING("nochaticon", false);
    ADD_SETTING("singlewatermark", false);
    ADD_SETTING("watermarkduplicates", true);
    ADD_SETTING("currentConfig", std::string("default.json"));
    ADD_SETTING("resettableSettings", true);
    ADD_SETTING("clearTextBoxWhenClicked", true);
    ADD_SETTING("dotcmdprefix", std::string("."));
    ADD_SETTING("autosearch", false);
}

void loadSettings() {
    auto& settings = ClientSettingsManager::instance();
    settings.checkSettingsFile();
    Client::privateInit = settings.isPrivateInitialized();
    if (Client::privateInit) {
        Client::LoadPrivate();
        Client::LoadSettings();
    }
    addClientSettings();
    Client::loadAvailableConfigs();
    Client::SavePrivate();
    if (!Client::privateInit) Client::LoadSettings();
}

bool ensureContext(ID3D11Device* device) {
    if (D2D::context && d2dFor == device) return true;
    D2D::context = nullptr;
    d2dDevice = nullptr;
    d2dFor = nullptr;
    winrt::com_ptr<IDXGIDevice> dxgi;
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(dxgi.put())))) return false;
    D2D1_CREATION_PROPERTIES props{D2D1_THREADING_MODE_SINGLE_THREADED, D2D1_DEBUG_LEVEL_NONE, D2D1_DEVICE_CONTEXT_OPTIONS_NONE};
    if (FAILED(D2D1CreateDevice(dxgi.get(), props, d2dDevice.put()))) return false;
    if (FAILED(d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, D2D::context.put()))) return false;
    d2dFor = device;
    return true;
}


HMODULE self = nullptr;

// What start() got through, so stop() undoes exactly that, also after a start that failed half way.
struct Stages {
    bool imgui = false;
    bool minhook = false;
    bool hooks = false;
    bool modules = false;
    bool commands = false;
};
Stages done;

void createFolders() {
    std::filesystem::path root = Utils::getClientPath();
    for (auto* sub : {"assets", "logs", "Config", "Crosshairs", "MessageLogger"}) {
        std::error_code ec;
        std::filesystem::create_directories(root / sub, ec);
        if (ec) Logger::warn("flarial core: could not create {}: {}", (root / sub).string(), ec.message());
    }
}

// Flarial's own start (Client::initialize) also pings Flarial's servers, migrates Flarial's folders, shows its
// menu hint and installs its DirectX hooks; Monchi does none of that, only what the game side needs.
bool start(const MonchiFlarialImGui* imgui) {
    ImGui::SetAllocatorFunctions(imgui->alloc, imgui->free, imgui->user);
    ImGui::SetCurrentContext(imgui->context);
    done.imgui = true;
    if (MH_Initialize() != MH_OK) return false;
    done.minhook = true;

    auto& window = WindowManager::instance();
    window.findGameWindow();
    Client::window = window.getWindow();
    Client::g_mainThreadId = window.getMainThreadId();
    Client::currentModule = self;
    PlatformUtils::initialize();
    try {
        winrt::init_apartment();
    } catch (...) {
    }

    VersionUtils::initialize();
    Client::version = WinrtUtils::impl::toRawString(WinrtUtils::impl::getGameVersion());
    InitializationManager::instance().setVersion(Client::version);
    if (!VersionUtils::isSupported(Client::version)) {
        Logger::warn("flarial core: game version {} is not supported", Client::version);
        return false;
    }
    VersionUtils::addData();
    createFolders();
    loadSettings();

    FlarialGUI::LoadFont(IDR_FONT_TTF);
    FlarialGUI::LoadFont(IDR_FONT_BOLD_TTF);
    FlarialGUI::LoadFont(IDR_MINECRAFTIA_TTF);

    done.hooks = true;
    HookManager::initialize();
    MH_ApplyQueued();
    ExpressionFormat::initialize();
    done.modules = true;
    ModuleManager::initialize();
    done.commands = true;
    CommandManager::initialize();
    Client::init = true;
    InitializationManager::instance().setInitialized(true);
    live = true;
    return true;
}

// Called inside Monchi's ImGui frame with Monchi's render target bound: Flarial's modules draw into the
// background draw list and, for some shapes and effects, through Direct2D on the same buffer.
// Flarial's effects (blur, motion blur, deepfry, images) read the device and swapchain from its own present hook.
// Monchi draws through D3D11 (11on12 on DX12 games), so Flarial is told it runs on D3D11 and takes those paths.
void adopt(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapchain) {
    if (SwapchainHook::d3d11Device.get() != device) {
        SwapchainHook::d3d11Device.copy_from(device);
        SwapchainHook::context.copy_from(context);
    }
    winrt::com_ptr<IDXGISwapChain3> chain;
    if (swapchain) swapchain->QueryInterface(IID_PPV_ARGS(chain.put()));
    SwapchainHook::swapchain = chain.get();
    SwapchainHook::isDX12 = false;
    SwapchainHook::initImgui = true;
    SwapchainHook::init = true;
}

// Ends the Direct2D pass whatever a module does in between, so a failing module never keeps the swapchain buffer
// bound (that breaks the next resize). A lost target drops the context, the next frame builds a new one.
struct DrawPass {
    bool open = false;

    void begin(ID2D1Bitmap1* bitmap) {
        SwapchainHook::D2D1Bitmap.copy_from(bitmap);
        D2D::context->SetTarget(bitmap);
        D2D::context->BeginDraw();
        open = true;
    }

    ~DrawPass() {
        if (!open) return;
        HRESULT hr = D2D::context->EndDraw();
        D2D::context->SetTarget(nullptr);
        SwapchainHook::D2D1Bitmap = nullptr;
        if (hr == D2DERR_RECREATE_TARGET) {
            D2D::context = nullptr;
            d2dDevice = nullptr;
            d2dFor = nullptr;
        }
    }
};

void frame(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapchain) {
    if (!live) return;
    adopt(device, context, swapchain);
    ID3D11RenderTargetView* rtv = nullptr;
    context->OMGetRenderTargets(1, &rtv, nullptr);
    if (!rtv) return;
    winrt::com_ptr<ID3D11RenderTargetView> target;
    target.attach(rtv);
    winrt::com_ptr<ID3D11Resource> resource;
    rtv->GetResource(resource.put());
    auto surface = resource.try_as<IDXGISurface>();
    if (!surface || !ensureContext(device)) return;

    D2D1_BITMAP_PROPERTIES1 props{};
    props.pixelFormat = {DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED};
    props.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
    winrt::com_ptr<ID2D1Bitmap1> bitmap;
    if (FAILED(D2D::context->CreateBitmapFromDxgiSurface(surface.get(), props, bitmap.put()))) return;

    DrawPass pass;
    pass.begin(bitmap.get());
    MC::windowSize = Vec2(D2D::context->GetSize().width, D2D::context->GetSize().height);
    ModuleManager::processPendingToggles();
    auto event = nes::make_holder<RenderEvent>();
    event->RTV = rtv;
    eventMgr.trigger(event);
}

// MinHook only keeps new calls out of disabled hooks; a thread already inside a Flarial detour finishes on its
// own. The pause gives those calls time to leave before the hook objects and modules go away.
constexpr DWORD hookDrainMs = 250;

bool stop() {
    live = false;
    if (done.minhook) {
        MH_DisableHook(MH_ALL_HOOKS);
        Sleep(hookDrainMs);
    }
    if (done.commands) CommandManager::terminate();
    if (done.modules) ModuleManager::terminate();
    if (done.hooks) HookManager::terminate();
    D2D::context = nullptr;
    d2dDevice = nullptr;
    d2dFor = nullptr;
    SwapchainHook::init = false;
    SwapchainHook::swapchain = nullptr;
    SwapchainHook::context = nullptr;
    SwapchainHook::d3d11Device = nullptr;
    if (done.minhook && MH_Uninitialize() != MH_OK) return false;
    if (done.imgui) ImGui::SetCurrentContext(nullptr);
    done = {};
    return true;
}

bool ejectRequested() { return live && Client::disable; }

}

extern "C" {
__declspec(dllexport) bool monchiFlarialStart(const MonchiFlarialImGui* imgui) { return start(imgui); }
__declspec(dllexport) void monchiFlarialFrame(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapchain) {
    frame(device, context, swapchain);
}
__declspec(dllexport) bool monchiFlarialStop() { return stop(); }
__declspec(dllexport) bool monchiFlarialEjectRequested() { return ejectRequested(); }
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        self = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}
