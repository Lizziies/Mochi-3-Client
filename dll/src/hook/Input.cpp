#include "Input.hpp"
#include "Hook.hpp"
#include "core/Client.hpp"
#include "core/Guard.hpp"
#include "system/Tweaks.hpp"
#include "core/Log.hpp"
#include "modules/Manager.hpp"
#include "render/Ui.hpp"

#include <windowsx.h>

#include <array>
#include <atomic>
#include <deque>
#include <mutex>

namespace input {

static HWND target = nullptr;
static WNDPROC original = nullptr;
static std::array<std::atomic<bool>, 256> keys{};
static std::mutex clickLock;
static std::deque<int64_t> clicks[2];
static std::atomic<int64_t> lastClick{0};
static std::atomic<int64_t> lastMove{0};
static std::atomic<int> motionX{0}, motionY{0};
static bool rawButtons = false;
static LARGE_INTEGER qpf{};

using ClipCursorFn = BOOL(WINAPI*)(const RECT*);
using SetCursorPosFn = BOOL(WINAPI*)(int, int);
static ClipCursorFn oClipCursor = nullptr;
static SetCursorPosFn oSetCursorPos = nullptr;

static int64_t qpc() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}

static void trimClicks(std::deque<int64_t>& d, int64_t t) {
    while (!d.empty() && t - d.front() > qpf.QuadPart) d.pop_front();
}

static void recordClick(int idx, int64_t t) {
    std::scoped_lock g(clickLock);
    clicks[idx].push_back(t);
    trimClicks(clicks[idx], t);
    lastClick = t;
}

static bool dispatchMouse(MouseEvent ev) {
    if (ev.button == MouseButton::Left) keys[VK_LBUTTON] = ev.down;
    if (ev.button == MouseButton::Right) keys[VK_RBUTTON] = ev.down;
    if (ev.button == MouseButton::Middle) keys[VK_MBUTTON] = ev.down;

    modules::dispatchMouse(ev);
    if (ev.cancel) return true;
    if (ev.down && (ev.button == MouseButton::Left || ev.button == MouseButton::Right))
        recordClick(ev.button == MouseButton::Left ? 0 : 1, ev.qpc);
    return false;
}

static bool handleRaw(LPARAM lp) {
    UINT size = 0;
    GetRawInputData(reinterpret_cast<HRAWINPUT>(lp), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
    if (size == 0 || size > 1024) return false;

    alignas(8) BYTE buf[1024];
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lp), RID_INPUT, buf, &size, sizeof(RAWINPUTHEADER)) != size)
        return false;

    auto* raw = reinterpret_cast<RAWINPUT*>(buf);
    if (raw->header.dwType != RIM_TYPEMOUSE) return false;

    auto& m = raw->data.mouse;
    int64_t t = qpc();
    bool cancel = false;

    if (!(m.usFlags & MOUSE_MOVE_ABSOLUTE) && (m.lLastX || m.lLastY)) {
        motionX += m.lLastX;
        motionY += m.lLastY;
        lastMove = t;
    }

    struct Map {
        USHORT down, up;
        MouseButton button;
    };
    static constexpr Map map[] = {
        {RI_MOUSE_LEFT_BUTTON_DOWN, RI_MOUSE_LEFT_BUTTON_UP, MouseButton::Left},
        {RI_MOUSE_RIGHT_BUTTON_DOWN, RI_MOUSE_RIGHT_BUTTON_UP, MouseButton::Right},
        {RI_MOUSE_MIDDLE_BUTTON_DOWN, RI_MOUSE_MIDDLE_BUTTON_UP, MouseButton::Middle},
    };
    for (auto& e : map) {
        if (m.usButtonFlags & e.down) {
            rawButtons = true;
            cancel |= dispatchMouse({e.button, true, 0, 0, 0, t});
        }
        if (m.usButtonFlags & e.up) cancel |= dispatchMouse({e.button, false, 0, 0, 0, t});
    }
    if (m.usButtonFlags & RI_MOUSE_WHEEL) {
        MouseEvent ev{MouseButton::None, false, (short)m.usButtonData, 0, 0, t};
        modules::dispatchMouse(ev);
        cancel |= ev.cancel;
    }
    return cancel;
}

static bool isMouseMessage(UINT msg) {
    return (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) || msg == WM_INPUT;
}

static bool isKeyMessage(UINT msg) {
    return msg >= WM_KEYFIRST && msg <= WM_KEYLAST;
}

static int resolveVk(WPARAM wp, LPARAM lp) {
    int vk = (int)wp;
    UINT scan = (lp >> 16) & 0xFF;
    bool extended = (lp >> 24) & 1;
    if (vk == VK_SHIFT) return (int)MapVirtualKeyW(scan, MAPVK_VSC_TO_VK_EX);
    if (vk == VK_CONTROL) return extended ? VK_RCONTROL : VK_LCONTROL;
    if (vk == VK_MENU) return extended ? VK_RMENU : VK_LMENU;
    return vk;
}

static bool process(HWND w, UINT msg, WPARAM wp, LPARAM lp, LRESULT& result) {
    if (client::unloading()) return false;

    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYUP) {
        bool isDown = msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN;
        int vk = resolveVk(wp, lp);
        bool repeat = isDown && ((lp >> 30) & 1);
        keys[vk & 0xFF] = isDown;
        if (vk != (int)wp) keys[wp & 0xFF] = isDown;

        if (isDown && !repeat && vk == 'L' && down(VK_CONTROL)) {
            client::requestUnload();
            return true;
        }

        KeyEvent ev{vk, isDown, repeat};
        modules::dispatchKey(ev);
        if (ev.cancel) return true;
    }

    int64_t t = qpc();
    switch (msg) {
    case WM_INPUT:
        if (handleRaw(lp)) {
            result = DefWindowProcW(w, msg, wp, lp);
            return true;
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        if (!rawButtons && dispatchMouse({msg == WM_LBUTTONDOWN ? MouseButton::Left : MouseButton::Right, true, 0, 0, 0, t}))
            return true;
        break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
        if (!rawButtons && dispatchMouse({msg == WM_LBUTTONUP ? MouseButton::Left : MouseButton::Right, false, 0, 0, 0, t}))
            return true;
        break;
    case WM_MOUSEWHEEL: {
        if (rawButtons) break;
        MouseEvent ev{MouseButton::None, false, GET_WHEEL_DELTA_WPARAM(wp), 0, 0, t};
        modules::dispatchMouse(ev);
        if (ev.cancel) return true;
        break;
    }
    case WM_KILLFOCUS:
        for (auto& k : keys) k = false;
        break;
    default:
        break;
    }

    if (ui::wndProc(w, msg, wp, lp)) {
        if (isMouseMessage(msg) || isKeyMessage(msg)) {
            result = msg == WM_INPUT ? DefWindowProcW(w, msg, wp, lp) : 0;
            return true;
        }
    }
    return false;
}

static LRESULT CALLBACK proc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    tweaks::threadBoost(tweaks::wantsInputBoost());
    LRESULT result = 0;
    bool handled = false;
    guard::call("wndproc", [&] { handled = process(w, msg, wp, lp, result); });
    if (handled) return result;
    return CallWindowProcW(original, w, msg, wp, lp);
}

static BOOL WINAPI clipCursor(const RECT* r) {
    if (ui::wantsCursor()) return oClipCursor(nullptr);
    return oClipCursor(r);
}

static BOOL WINAPI setCursorPos(int x, int y) {
    if (ui::wantsCursor()) return TRUE;
    return oSetCursorPos(x, y);
}

bool install(HWND window) {
    QueryPerformanceFrequency(&qpf);
    target = window;
    original = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(proc)));
    if (!original) {
        logger::error("wndproc subclass failed: {}", GetLastError());
        return false;
    }

    hook::create("ClipCursor", hook::exported(L"user32.dll", "ClipCursor"), clipCursor, &oClipCursor);
    hook::create("SetCursorPos", hook::exported(L"user32.dll", "SetCursorPos"), setCursorPos, &oSetCursorPos);
    hook::enableAll();
    return true;
}

void uninstall() {
    if (target && original) SetWindowLongPtrW(target, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(original));
    original = nullptr;
}

bool down(int vk) {
    if (vk == VK_CONTROL) return keys[VK_LCONTROL] || keys[VK_RCONTROL] || keys[VK_CONTROL];
    if (vk == VK_SHIFT) return keys[VK_LSHIFT] || keys[VK_RSHIFT];
    if (vk == VK_MENU) return keys[VK_LMENU] || keys[VK_RMENU];
    return keys[vk & 0xFF];
}

int cps(MouseButton button) {
    int idx = button == MouseButton::Left ? 0 : 1;
    std::scoped_lock g(clickLock);
    trimClicks(clicks[idx], qpc());
    return (int)clicks[idx].size();
}

int64_t lastClickQpc() { return lastClick; }
int64_t lastMoveQpc() { return lastMove; }

void consumeMotion(int& dx, int& dy) {
    dx = motionX.exchange(0);
    dy = motionY.exchange(0);
}

}
