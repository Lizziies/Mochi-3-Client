#include "Dx.hpp"
#include "Hook.hpp"
#include "core/Guard.hpp"
#include "core/Log.hpp"
#include "render/Ui.hpp"

#include <d3d11.h>
#include <d3d11on12.h>
#include <d3d12.h>
#include <dxgi1_5.h>

#include <atomic>
#include <mutex>
#include <vector>

namespace dx {

using PresentFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
using Present1Fn = HRESULT(WINAPI*)(IDXGISwapChain1*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*);
using ResizeFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
using ExecuteFn = void(WINAPI*)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);

static PresentFn oPresent = nullptr;
static Present1Fn oPresent1 = nullptr;
static ResizeFn oResize = nullptr;
static ExecuteFn oExecute = nullptr;

static Api current = Api::None;
static HWND hwnd = nullptr;
static Tuning tune;
static FrameInfo info;
static thread_local bool inPresent = false;
static std::atomic<bool> dead{false};

static ID3D11Device* d11 = nullptr;
static ID3D11DeviceContext* ctx = nullptr;
static ID3D11On12Device* on12 = nullptr;
static ID3D12CommandQueue* queue = nullptr;
static IDXGISwapChain* chain = nullptr;
static bool uiReady = false;
static bool latencyApplied = false;
static int setupCooldown = 0;
static int setupFailures = 0;

struct Buffer {
    ID3D11Resource* wrapped = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
};
static ID3D11RenderTargetView* rtv11 = nullptr;

static LARGE_INTEGER qpf{};
static int64_t lastPresent = 0;
static HANDLE limiterTimer = nullptr;

template <class T>
static void release(T*& p) {
    if (p) p->Release();
    p = nullptr;
}

static int64_t now() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}

static ID3D12Fence* fence = nullptr;
static HANDLE fenceEvent = nullptr;
static UINT64 fenceValue = 0;

// 11on12 keeps its references to the swapchain buffers until the queue has finished the work that used them,
// and ResizeBuffers fails (the game then aborts) while any reference is left
static void waitGpu() {
    if (!queue) return;
    if (!fence) {
        ID3D12Device* device = nullptr;
        if (FAILED(queue->GetDevice(IID_PPV_ARGS(&device)))) return;
        HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        device->Release();
        if (FAILED(hr)) return;
        fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    }
    if (!fenceEvent) return;
    UINT64 value = ++fenceValue;
    if (FAILED(queue->Signal(fence, value)) || fence->GetCompletedValue() >= value) return;
    if (SUCCEEDED(fence->SetEventOnCompletion(value, fenceEvent))) WaitForSingleObject(fenceEvent, 2000);
}

static void dropTargets() {
    release(rtv11);
    if (ctx) {
        ctx->ClearState();
        ctx->Flush();
    }
    if (current == Api::Dx12) waitGpu();
}

static void releaseBuffer(Buffer& b) {
    release(b.rtv);
    release(b.wrapped);
}

static bool wrapBuffer(IDXGISwapChain* sc, UINT index, Buffer& out) {
    DXGI_SWAP_CHAIN_DESC desc{};
    sc->GetDesc(&desc);
    info.bufferCount = (int)desc.BufferCount;

    ID3D12Resource* res = nullptr;
    if (FAILED(sc->GetBuffer(index, IID_PPV_ARGS(&res)))) return false;

    D3D11_RESOURCE_FLAGS flags{D3D11_BIND_RENDER_TARGET};
    HRESULT hr = on12->CreateWrappedResource(res, &flags, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT,
                                             IID_PPV_ARGS(&out.wrapped));
    res->Release();
    if (FAILED(hr)) return false;
    return SUCCEEDED(d11->CreateRenderTargetView(out.wrapped, nullptr, &out.rtv));
}

static bool buildTargets11(IDXGISwapChain* sc) {
    ID3D11Texture2D* back = nullptr;
    if (FAILED(sc->GetBuffer(0, IID_PPV_ARGS(&back)))) return false;
    HRESULT hr = d11->CreateRenderTargetView(back, nullptr, &rtv11);
    back->Release();
    DXGI_SWAP_CHAIN_DESC desc{};
    sc->GetDesc(&desc);
    info.bufferCount = (int)desc.BufferCount;
    return SUCCEEDED(hr);
}

static HWND findWindow(IDXGISwapChain* sc) {
    DXGI_SWAP_CHAIN_DESC desc{};
    if (SUCCEEDED(sc->GetDesc(&desc)) && desc.OutputWindow) return desc.OutputWindow;

    IDXGISwapChain1* sc1 = nullptr;
    HWND out = nullptr;
    if (SUCCEEDED(sc->QueryInterface(IID_PPV_ARGS(&sc1)))) {
        sc1->GetHwnd(&out);
        sc1->Release();
    }
    if (out) return out;

    struct Search {
        DWORD pid;
        HWND best;
        long area;
    } s{GetCurrentProcessId(), nullptr, 0};

    EnumWindows([](HWND w, LPARAM lp) -> BOOL {
        auto* s = reinterpret_cast<Search*>(lp);
        DWORD pid = 0;
        GetWindowThreadProcessId(w, &pid);
        if (pid != s->pid || !IsWindowVisible(w)) return TRUE;
        RECT r{};
        GetClientRect(w, &r);
        long area = (r.right - r.left) * (r.bottom - r.top);
        if (area > s->area) {
            s->area = area;
            s->best = w;
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&s));
    return s.best;
}

static bool setup(IDXGISwapChain* sc) {
    ID3D12Device* d12 = nullptr;
    if (SUCCEEDED(sc->GetDevice(IID_PPV_ARGS(&d12)))) {
        ID3D12CommandQueue* own = nullptr;
        if (SUCCEEDED(sc->GetDevice(IID_PPV_ARGS(&own)))) {
            release(queue);
            queue = own;
        }
        if (!queue) {
            d12->Release();
            return false;
        }

        IUnknown* queues[] = {queue};
        HRESULT hr = D3D11On12CreateDevice(d12, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, queues, 1, 0, &d11,
                                           &ctx, nullptr);
        d12->Release();
        if (FAILED(hr)) {
            logger::error("D3D11On12CreateDevice failed 0x{:08X}", (unsigned)hr);
            return false;
        }
        d11->QueryInterface(IID_PPV_ARGS(&on12));
        current = Api::Dx12;
    } else if (SUCCEEDED(sc->GetDevice(IID_PPV_ARGS(&d11)))) {
        d11->GetImmediateContext(&ctx);
        current = Api::Dx11;
    } else {
        return false;
    }

    hwnd = findWindow(sc);
    chain = sc;
    logger::info("renderer: {} hwnd={}", current == Api::Dx12 ? "dx12 (11on12)" : "dx11", (void*)hwnd);

    DXGI_SWAP_CHAIN_DESC desc{};
    sc->GetDesc(&desc);
    info.tearingSupported = (desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) != 0;

    uiReady = ui::init(hwnd, d11, ctx);
    return uiReady;
}

static void applyLatency(IDXGISwapChain* sc) {
    if (latencyApplied == tune.lowLatency) return;
    latencyApplied = tune.lowLatency;
    UINT frames = tune.lowLatency ? 1 : 3;
    info.lowLatencyActive = false;

    DXGI_SWAP_CHAIN_DESC desc{};
    sc->GetDesc(&desc);
    if (desc.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT) {
        IDXGISwapChain2* sc2 = nullptr;
        if (SUCCEEDED(sc->QueryInterface(IID_PPV_ARGS(&sc2)))) {
            info.lowLatencyActive = SUCCEEDED(sc2->SetMaximumFrameLatency(frames)) && tune.lowLatency;
            sc2->Release();
        }
        return;
    }
    if (current == Api::Dx11) {
        IDXGIDevice1* dev = nullptr;
        if (SUCCEEDED(d11->QueryInterface(IID_PPV_ARGS(&dev)))) {
            info.lowLatencyActive = SUCCEEDED(dev->SetMaximumFrameLatency(frames)) && tune.lowLatency;
            dev->Release();
        }
    }
}

static void limit() {
    if (tune.fpsLimit < 10.f) return;
    if (!limiterTimer)
        limiterTimer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

    int64_t period = (int64_t)(qpf.QuadPart / tune.fpsLimit);
    int64_t target = lastPresent + period;
    int64_t t = now();
    if (t >= target) return;

    int64_t remaining = target - t;
    int64_t spin = qpf.QuadPart / 2000;
    if (remaining > spin && limiterTimer) {
        LARGE_INTEGER due;
        due.QuadPart = -((remaining - spin) * 10000000 / qpf.QuadPart);
        SetWaitableTimer(limiterTimer, &due, 0, nullptr, nullptr, FALSE);
        WaitForSingleObject(limiterTimer, INFINITE);
    }
    while (now() < target) YieldProcessor();
}

static void draw(IDXGISwapChain* sc) {
    if (dead) return;
    if (!uiReady) {
        if (setupCooldown > 0) {
            setupCooldown--;
            return;
        }
        if (!setup(sc)) {
            release(on12);
            release(ctx);
            release(d11);
            setupCooldown = 120;
            if (++setupFailures >= 10) {
                logger::error("renderer setup failed {} times, giving up", setupFailures);
                dead = true;
            }
            return;
        }
    }
    if (sc != chain) return;

    applyLatency(sc);

    if (current == Api::Dx12) {
        IDXGISwapChain3* sc3 = nullptr;
        if (FAILED(sc->QueryInterface(IID_PPV_ARGS(&sc3)))) return;
        UINT idx = sc3->GetCurrentBackBufferIndex();
        sc3->Release();

        // wrapped for this frame only: nothing may hold a swapchain buffer between frames, the game resizes,
        // toggles fullscreen and recreates its swapchain behind our back
        Buffer b;
        if (!wrapBuffer(sc, idx, b)) {
            releaseBuffer(b);
            return;
        }
        on12->AcquireWrappedResources(&b.wrapped, 1);
        ctx->OMSetRenderTargets(1, &b.rtv, nullptr);
        ui::frame();
        ctx->OMSetRenderTargets(0, nullptr, nullptr);
        on12->ReleaseWrappedResources(&b.wrapped, 1);
        ctx->ClearState();
        ctx->Flush();
        releaseBuffer(b);
        ctx->Flush();
    } else {
        if (!rtv11 && !buildTargets11(sc)) return;
        ctx->OMSetRenderTargets(1, &rtv11, nullptr);
        ui::frame();
    }
}

static void beforePresent(IDXGISwapChain* sc, UINT& sync, UINT& flags) {
    guard::call("present", [&] { draw(sc); });

    if (tune.allowTearing && info.tearingSupported && !(flags & DXGI_PRESENT_TEST)) {
        BOOL fullscreen = FALSE;
        sc->GetFullscreenState(&fullscreen, nullptr);
        if (!fullscreen) {
            sync = 0;
            flags |= DXGI_PRESENT_ALLOW_TEARING;
        }
    }
    limit();
}

static void afterPresent() {
    int64_t t = now();
    if (lastPresent) info.frameMs = double(t - lastPresent) * 1000.0 / double(qpf.QuadPart);
    lastPresent = t;
    info.presentQpc = t;
}

static HRESULT WINAPI present(IDXGISwapChain* sc, UINT sync, UINT flags) {
    if (inPresent) return oPresent(sc, sync, flags);
    inPresent = true;
    beforePresent(sc, sync, flags);
    HRESULT hr = oPresent(sc, sync, flags);
    afterPresent();
    inPresent = false;
    return hr;
}

static HRESULT WINAPI present1(IDXGISwapChain1* sc, UINT sync, UINT flags, const DXGI_PRESENT_PARAMETERS* params) {
    if (inPresent) return oPresent1(sc, sync, flags, params);
    inPresent = true;
    beforePresent(sc, sync, flags);
    HRESULT hr = oPresent1(sc, sync, flags, params);
    afterPresent();
    inPresent = false;
    return hr;
}

static HRESULT WINAPI resize(IDXGISwapChain* sc, UINT count, UINT w, UINT h, DXGI_FORMAT fmt, UINT flags) {
    if (sc == chain) {
        dropTargets();
        ui::invalidate();
    }
    HRESULT hr = oResize(sc, count, w, h, fmt, flags);
    if (sc == chain) {
        DXGI_SWAP_CHAIN_DESC desc{};
        sc->GetDesc(&desc);
        info.tearingSupported = (desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) != 0;
        latencyApplied = !tune.lowLatency;
    }
    return hr;
}

static void WINAPI execute(ID3D12CommandQueue* q, UINT n, ID3D12CommandList* const* lists) {
    if (!queue) {
        D3D12_COMMAND_QUEUE_DESC d = q->GetDesc();
        if (d.Type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
            q->AddRef();
            queue = q;
        }
    }
    oExecute(q, n, lists);
}

static LRESULT CALLBACK dummyProc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    return DefWindowProcW(w, m, wp, lp);
}

bool install() {
    QueryPerformanceFrequency(&qpf);

    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = dummyProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"MochiDummy";
    RegisterClassExW(&wc);
    HWND tmp = CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr,
                               wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = tmp;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* sc = nullptr;
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* dc = nullptr;
    D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &fl, 1,
                                               D3D11_SDK_VERSION, &sd, &sc, &dev, nullptr, &dc);
    if (FAILED(hr)) {
        hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, &fl, 1, D3D11_SDK_VERSION, &sd,
                                           &sc, &dev, nullptr, &dc);
    }
    if (FAILED(hr)) {
        logger::error("dummy swapchain failed 0x{:08X}", (unsigned)hr);
        DestroyWindow(tmp);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }

    bool ok = hook::create("Present", hook::vfunc(sc, 8), present, &oPresent) &&
              hook::create("ResizeBuffers", hook::vfunc(sc, 13), resize, &oResize);

    IDXGISwapChain1* sc1 = nullptr;
    if (SUCCEEDED(sc->QueryInterface(IID_PPV_ARGS(&sc1)))) {
        hook::create("Present1", hook::vfunc(sc1, 22), present1, &oPresent1);
        sc1->Release();
    }

    ID3D12Device* d12 = nullptr;
    if (SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d12)))) {
        D3D12_COMMAND_QUEUE_DESC qd{};
        ID3D12CommandQueue* q = nullptr;
        if (SUCCEEDED(d12->CreateCommandQueue(&qd, IID_PPV_ARGS(&q)))) {
            hook::create("ExecuteCommandLists", hook::vfunc(q, 10), execute, &oExecute);
            q->Release();
        }
        d12->Release();
    }

    sc->Release();
    dc->Release();
    dev->Release();
    DestroyWindow(tmp);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return ok;
}

void uninstall() {
    dead = true;
    if (uiReady) ui::shutdown();
    uiReady = false;
    dropTargets();
    release(on12);
    release(ctx);
    release(d11);
    release(fence);
    if (fenceEvent) CloseHandle(fenceEvent);
    fenceEvent = nullptr;
    release(queue);
    if (limiterTimer) CloseHandle(limiterTimer);
    limiterTimer = nullptr;
}

Api api() { return current; }
HWND window() { return hwnd; }
Tuning& tuning() { return tune; }
const FrameInfo& frame() { return info; }

}
