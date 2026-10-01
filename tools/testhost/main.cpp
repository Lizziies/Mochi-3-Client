#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>

static HWND hwnd;
static int viewW = 1280, viewH = 720;
static bool running = true;

static LRESULT CALLBACK proc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    if (m == WM_DESTROY || m == WM_CLOSE) {
        running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(w, m, wp, lp);
}

static void key(int vk, bool down) {
    PostMessageW(hwnd, down ? WM_KEYDOWN : WM_KEYUP, vk, 1);
}

struct Queued {
    DWORD due;
    UINT msg;
    WPARAM wp;
    LPARAM lp;
};
static std::vector<Queued> queued;

static void click(int x, int y) {
    LPARAM lp = MAKELPARAM(x, y);
    DWORD now = GetTickCount();
    queued.push_back({now, WM_MOUSEMOVE, 0, lp});
    queued.push_back({now + 150, WM_LBUTTONDOWN, MK_LBUTTON, lp});
    queued.push_back({now + 260, WM_LBUTTONUP, 0, lp});
}

static void flushQueued() {
    DWORD now = GetTickCount();
    for (size_t i = 0; i < queued.size();) {
        if (queued[i].due <= now) {
            PostMessageW(hwnd, queued[i].msg, queued[i].wp, queued[i].lp);
            queued.erase(queued.begin() + i);
        } else i++;
    }
}

int wmain(int argc, wchar_t** argv) {
    if (argc < 2) {
        std::printf("usage: testhost <Mochi.dll> [seconds]\n");
        return 1;
    }
    int seconds = argc > 2 ? _wtoi(argv[2]) : 26;

    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = proc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"TestHost";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    if (const char* size = std::getenv("TESTHOST_SIZE")) std::sscanf(size, "%dx%d", &viewW, &viewH);
    bool manual = std::getenv("TESTHOST_MANUAL") != nullptr;
    RECT r{0, 0, viewW, viewH};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    hwnd = CreateWindowExW(0, wc.lpszClassName, L"TestHost", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, r.right - r.left,
                           r.bottom - r.top, nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = viewW;
    sd.BufferDesc.Height = viewH;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* sc = nullptr;
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &fl, 1, D3D11_SDK_VERSION,
                                               &sd, &sc, &dev, nullptr, &ctx);
    if (FAILED(hr)) {
        std::printf("device failed 0x%08lx\n", (unsigned long)hr);
        return 2;
    }

    ID3D11Texture2D* back = nullptr;
    sc->GetBuffer(0, IID_PPV_ARGS(&back));
    ID3D11RenderTargetView* rtv = nullptr;
    dev->CreateRenderTargetView(back, nullptr, &rtv);
    back->Release();

    HMODULE mochi = LoadLibraryW(argv[1]);
    std::printf("LoadLibrary -> %p (err %lu)\n", (void*)mochi, mochi ? 0 : GetLastError());
    std::fflush(stdout);

    DWORD start = GetTickCount();
    int step = 0;
    int frames = 0;
    while (running) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        flushQueued();
        double t = (GetTickCount() - start) / 1000.0;

        float sky = 0.5f + 0.1f * std::sin(t);
        float color[4] = {0.35f, sky, 0.9f, 1.f};
        ctx->OMSetRenderTargets(1, &rtv, nullptr);
        D3D11_VIEWPORT vp{0, 0, (float)viewW, (float)viewH, 0, 1};
        ctx->RSSetViewports(1, &vp);
        ctx->ClearRenderTargetView(rtv, color);

        sc->Present(0, 0);
        frames++;

        struct Action {
            double at;
            const char* name;
            void (*run)();
        };
        static const Action actions[] = {
            {3, "open", [] { key(VK_RSHIFT, true); key(VK_RSHIFT, false); }},
            {6, "card", [] { click(580, 255); }},
            {9, "themes", [] { click(333, 248); }},
            {12, "settings", [] { click(325, 306); }},
            {15, "hudedit", [] { click(365, 557); }},
            {18, "back", [] { key(VK_ESCAPE, true); key(VK_ESCAPE, false); }},
            {21, "unload", [] { key(VK_CONTROL, true); key('L', true); key('L', false); key(VK_CONTROL, false); }},
        };
        if (!manual && step < (int)(sizeof(actions) / sizeof(actions[0])) && t > actions[step].at) {
            actions[step].run();
            std::printf("action %s\n", actions[step].name);
            std::fflush(stdout);
            step++;
        }
        if (t > seconds) break;
        Sleep(4);
    }
    std::printf("frames=%d still_loaded=%d\n", frames, GetModuleHandleW(L"Mochi.dll") != nullptr);
    return 0;
}
