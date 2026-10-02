#include "GameInput.hpp"
#include "Hook.hpp"
#include "core/Log.hpp"
#include "render/Ui.hpp"

#include <windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <cmath>
#include <cstring>
#include <mutex>
#include <string>
#include <utility>

namespace gameinput {

namespace {

// Slots and layouts come from the public GameInput.h of every API version (v0 to v3). IGameInput keeps
// GetCurrentReading at slot 4 in all of them. IGameInputReading lost GetSequenceNumber and GetRawReport after
// v0, so its key and mouse getters sit two slots lower from v1 on, and v1 added the positions field and the
// absolute position pair to GameInputMouseState, which moves the wheel values back.
constexpr int currentReadingSlot = 4;
constexpr unsigned kindKeyboard = 0x10;
constexpr unsigned kindMouse = 0x20;
constexpr int maxKeys = 64;
constexpr int maxHooks = 12;

struct KeyState {
    uint32_t scanCode;
    uint32_t codePoint;
    uint8_t virtualKey;
    bool isDeadKey;
};

struct Layout {
    int count, keys, mouse, device, wheel;
};

constexpr Layout layoutV0{14, 15, 16, 6, 24};
constexpr Layout layoutV1{12, 13, 14, 5, 40};

struct Api {
    const char* name;
    GUID iid;
    const Layout* layout;
};

const Api apis[] = {
    {"v3", {0x20EFC1C7, 0x5D9A, 0x43BA, {0xB2, 0x6F, 0xB8, 0x07, 0xFA, 0x48, 0x60, 0x9C}}, &layoutV1},
    {"v2", {0xBBAA66D2, 0x837A, 0x40F7, {0xA3, 0x03, 0x91, 0x7D, 0x50, 0x09, 0x55, 0xF4}}, &layoutV1},
    {"v1", {0x40FFB7E4, 0x6150, 0x407A, {0xB4, 0x39, 0x13, 0x2B, 0xAD, 0xC0, 0x8D, 0x2D}}, &layoutV1},
    {"v0", {0x11BE2A7E, 0x4254, 0x445A, {0x9C, 0x09, 0xFF, 0xC4, 0x0F, 0x00, 0x69, 0x18}}, &layoutV0},
};

using InitFn = HRESULT(WINAPI*)(REFIID, void**);
using CreateFn = HRESULT(WINAPI*)(void**);
using QueryFn = HRESULT(STDMETHODCALLTYPE*)(void*, REFIID, void**);
using RefFn = ULONG(STDMETHODCALLTYPE*)(void*);
using CurrentFn = HRESULT(STDMETHODCALLTYPE*)(void*, unsigned, void*, void**);
using CountFn = uint32_t(STDMETHODCALLTYPE*)(void*);
using KeysFn = uint32_t(STDMETHODCALLTYPE*)(void*, uint32_t, KeyState*);
using MouseFn = bool(STDMETHODCALLTYPE*)(void*, uint8_t*);
using DeviceFn = void(STDMETHODCALLTYPE*)(void*, void**);

struct Known {
    void** vtable = nullptr;
    const Layout* layout = nullptr;
    bool deviceRefs = false;
};

enum class Phase { Searching, Hooked, Absent };

std::atomic<Phase> phase{Phase::Searching};
std::array<Known, 16> known{};
std::atomic<int> knownCount{0};
std::array<void*, maxHooks> originals{};
std::array<void*, maxHooks> targets{};
int hookCount = 0;
std::string summary = "searching";

thread_local int depth = 0;

struct Nest {
    Nest() { depth++; }
    ~Nest() { depth--; }
};

ULONG addRef(void* o) { return reinterpret_cast<RefFn>(hook::vfunc(o, 1))(o); }
ULONG release(void* o) { return reinterpret_cast<RefFn>(hook::vfunc(o, 2))(o); }

const Known* knownOf(void* self) {
    void** vt = *static_cast<void***>(self);
    int n = knownCount.load(std::memory_order_acquire);
    for (int i = 0; i < n; i++)
        if (known[i].vtable == vt) return &known[i];
    return nullptr;
}

std::mutex overlayLock;
std::bitset<256> pendingHold, pendingDrop, activeHold, activeDrop;
float pendingScale = 1.f;
bool pendingWheel = false;
std::atomic<float> activeScale{1.f};
std::atomic<bool> activeWheel{false};

std::mutex keyLock;
std::bitset<256> stale;
bool keysBlocked = false;
std::array<uint8_t, 512> learnedVk{};
int loggedKeys = 0;

int scanKey(uint32_t scan) { return int((scan & 0xFF) | ((scan & 0xE000) ? 0x100 : 0)); }

uint32_t scanOf(int vk) {
    UINT sc = MapVirtualKeyW(UINT(vk), MAPVK_VK_TO_VSC_EX);
    return sc;
}

bool blockedNow() { return ui::capturing(); }

bool dropped(const KeyState& k, const std::bitset<256>& drops) {
    if (drops[k.virtualKey]) return true;
    for (int vk = 0; vk < 256; vk++)
        if (drops[vk] && scanKey(scanOf(vk)) == scanKey(k.scanCode)) return true;
    return false;
}

void learn(const KeyState& k) {
    int s = scanKey(k.scanCode);
    if (learnedVk[s] == k.virtualKey) return;
    learnedVk[s] = k.virtualKey;
    if (loggedKeys < 12) {
        loggedKeys++;
        logger::info("gameinput: key scan=0x{:X} vk=0x{:02X}", k.scanCode, k.virtualKey);
    }
}

uint32_t filter(KeyState* keys, uint32_t n, uint32_t cap) {
    std::scoped_lock g(keyLock);
    if (blockedNow()) {
        keysBlocked = true;
        return 0;
    }
    std::bitset<256> present;
    for (uint32_t i = 0; i < n; i++) {
        present.set(keys[i].virtualKey);
        learn(keys[i]);
    }
    if (keysBlocked) {
        keysBlocked = false;
        stale = present;
    }
    stale &= present;

    std::bitset<256> holds, drops;
    {
        std::scoped_lock o(overlayLock);
        holds = activeHold;
        drops = activeDrop;
    }
    uint32_t w = 0;
    for (uint32_t i = 0; i < n; i++) {
        if (stale[keys[i].virtualKey] || dropped(keys[i], drops)) continue;
        keys[w++] = keys[i];
    }
    for (int vk = 0; vk < 256 && w < cap; vk++) {
        if (!holds[vk]) continue;
        uint32_t sc = scanOf(vk);
        uint8_t gameVk = learnedVk[scanKey(sc)] ? learnedVk[scanKey(sc)] : uint8_t(vk);
        bool there = false;
        for (uint32_t i = 0; i < w && !there; i++) there = keys[i].virtualKey == gameVk || scanKey(keys[i].scanCode) == scanKey(sc);
        if (!there) keys[w++] = {sc, 0, gameVk, false};
    }
    return w;
}

uint32_t onKeys(int slot, void* self, uint32_t max, KeyState* out) {
    auto original = reinterpret_cast<KeysFn>(originals[slot]);
    if (depth || !out) return original(self, max, out);
    Nest nest;
    KeyState keys[maxKeys + 16];
    uint32_t n = std::min<uint32_t>(original(self, maxKeys, keys), maxKeys);
    n = filter(keys, n, maxKeys + 16);
    uint32_t c = std::min(n, max);
    std::memcpy(out, keys, c * sizeof(KeyState));
    return c;
}

uint32_t onCount(int slot, void* self) {
    auto original = reinterpret_cast<CountFn>(originals[slot]);
    if (depth) return original(self);
    const Known* k = knownOf(self);
    if (!k) return original(self);
    Nest nest;
    KeyState keys[maxKeys + 16];
    auto read = reinterpret_cast<KeysFn>(hook::vfunc(self, k->layout->keys));
    uint32_t n = std::min<uint32_t>(read(self, maxKeys, keys), maxKeys);
    return filter(keys, n, maxKeys + 16);
}

struct Mouse {
    void* id = nullptr;
    bool used = false;
    bool blocked = false;
    int64_t raw[4]{};
    int64_t out[4]{};
    double frac[2]{};
    uint32_t stale = 0;
};

std::mutex mouseLock;
std::array<Mouse, 8> mice;
int wheelLogs = 0;

Mouse& mouseFor(void* id) {
    for (auto& m : mice)
        if (m.used && m.id == id) return m;
    for (auto& m : mice)
        if (!m.used) {
            m = {};
            m.id = id;
            return m;
        }
    mice[0] = {};
    mice[0].id = id;
    return mice[0];
}

void adjust(void* id, uint8_t* state, int wheel) {
    std::scoped_lock g(mouseLock);
    auto* buttons = reinterpret_cast<uint32_t*>(state);
    int64_t* v[4] = {reinterpret_cast<int64_t*>(state + 8), reinterpret_cast<int64_t*>(state + 16),
                     reinterpret_cast<int64_t*>(state + wheel), reinterpret_cast<int64_t*>(state + wheel + 8)};
    bool blocked = blockedNow();
    bool wheelHeld = blocked || activeWheel.load();
    float scale = activeScale.load();
    Mouse& m = mouseFor(id);
    if (!m.used) {
        m.used = true;
        for (int i = 0; i < 4; i++) m.raw[i] = m.out[i] = *v[i];
    }
    for (int i = 0; i < 4; i++) {
        int64_t delta = *v[i] - m.raw[i];
        if (i >= 2 && delta && wheelLogs < 3) {
            wheelLogs++;
            logger::info("gameinput: wheel {} -> {}", m.raw[i], *v[i]);
        }
        m.raw[i] = *v[i];
        if (blocked || (i >= 2 && wheelHeld)) delta = 0;
        else if (i < 2 && scale != 1.f) {
            double f = double(delta) * scale + m.frac[i];
            delta = int64_t(std::floor(f));
            m.frac[i] = f - double(delta);
        }
        m.out[i] += delta;
        *v[i] = m.out[i];
    }
    if (blocked) {
        m.blocked = true;
        *buttons = 0;
        return;
    }
    if (m.blocked) {
        m.blocked = false;
        m.stale = *buttons;
    }
    m.stale &= *buttons;
    *buttons &= ~m.stale;
}

bool onMouse(int slot, void* self, uint8_t* state) {
    auto original = reinterpret_cast<MouseFn>(originals[slot]);
    bool ok = original(self, state);
    if (depth || !ok || !state) return ok;
    const Known* k = knownOf(self);
    if (!k) return ok;
    Nest nest;
    void* device = nullptr;
    reinterpret_cast<DeviceFn>(hook::vfunc(self, k->layout->device))(self, &device);
    adjust(device, state, k->layout->wheel);
    if (device && k->deviceRefs) release(device);
    return ok;
}

template <int N>
uint32_t STDMETHODCALLTYPE countDetour(void* self) {
    return onCount(N, self);
}

template <int N>
uint32_t STDMETHODCALLTYPE keysDetour(void* self, uint32_t max, KeyState* out) {
    return onKeys(N, self, max, out);
}

template <int N>
bool STDMETHODCALLTYPE mouseDetour(void* self, uint8_t* state) {
    return onMouse(N, self, state);
}

enum Role { RoleCount, RoleKeys, RoleMouse };

template <int... N>
std::array<void*, 3 * maxHooks> detourTable(std::integer_sequence<int, N...>) {
    return {reinterpret_cast<void*>(&countDetour<N>)..., reinterpret_cast<void*>(&keysDetour<N>)...,
            reinterpret_cast<void*>(&mouseDetour<N>)...};
}

const auto detours = detourTable(std::make_integer_sequence<int, maxHooks>{});

bool hookOnce(void* target, Role role, const char* api) {
    for (int i = 0; i < hookCount; i++)
        if (targets[i] == target) return true;
    if (!target || hookCount >= maxHooks) return false;
    int slot = hookCount;
    static const char* names[] = {"GetKeyCount", "GetKeyState", "GetMouseState"};
    std::string name = std::string("GameInput ") + api + " " + names[role];
    if (!hook::create(name.c_str(), target, detours[role * maxHooks + slot], &originals[slot])) return false;
    targets[slot] = target;
    hookCount++;
    return true;
}

bool deviceAddsRef(void* reading, const Layout& layout) {
    auto device = reinterpret_cast<DeviceFn>(hook::vfunc(reading, layout.device));
    void* first = nullptr;
    device(reading, &first);
    if (!first) return false;
    ULONG a = addRef(first);
    release(first);
    void* second = nullptr;
    device(reading, &second);
    ULONG b = second ? addRef(second) : a;
    if (second) release(second);
    bool refs = b == a + 1;
    if (refs) {
        release(first);
        release(second);
    }
    return refs;
}

bool study(void* gi, unsigned kind, const Api& api) {
    void* reading = nullptr;
    auto current = reinterpret_cast<CurrentFn>(hook::vfunc(gi, currentReadingSlot));
    if (FAILED(current(gi, kind, nullptr, &reading)) || !reading) return false;
    void** vt = *static_cast<void***>(reading);
    bool fresh = true;
    int n = knownCount.load();
    for (int i = 0; i < n; i++)
        if (known[i].vtable == vt) fresh = false;
    bool ok = true;
    if (fresh && n < int(known.size())) {
        known[n] = {vt, api.layout, deviceAddsRef(reading, *api.layout)};
        knownCount.store(n + 1, std::memory_order_release);
        const Layout& l = *api.layout;
        ok = hookOnce(vt[l.count], RoleCount, api.name) && hookOnce(vt[l.keys], RoleKeys, api.name) &&
             hookOnce(vt[l.mouse], RoleMouse, api.name);
    }
    release(reading);
    return ok;
}

HMODULE runtime() {
    if (HMODULE m = GetModuleHandleW(L"GameInputRedist.dll")) return m;
    return GetModuleHandleW(L"GameInput.dll");
}

bool attach() {
    HMODULE m = runtime();
    if (!m) return false;
    auto init = reinterpret_cast<InitFn>(GetProcAddress(m, "GameInputInitialize"));
    auto create = reinterpret_cast<CreateFn>(GetProcAddress(m, "GameInputCreate"));
    void* root = nullptr;
    if (!init && (!create || FAILED(create(&root)) || !root)) return false;

    std::string found;
    bool keyboard = false, mouse = false;
    for (auto& api : apis) {
        void* gi = nullptr;
        if (init) init(api.iid, &gi);
        else reinterpret_cast<QueryFn>(hook::vfunc(root, 0))(root, api.iid, &gi);
        if (!gi) continue;
        bool k = study(gi, kindKeyboard, api);
        bool ms = study(gi, kindMouse, api);
        keyboard |= k;
        mouse |= ms;
        if (k || ms) found += std::string(found.empty() ? "" : ", ") + api.name;
        release(gi);
    }
    if (root) release(root);
    if (!keyboard || !mouse) return false;
    hook::enableAll();
    summary = "hooked (" + found + ")";
    logger::info("gameinput: readings {} with {} hooks, menu now blocks keyboard and mouse", summary, hookCount);
    return true;
}

}

void tick() {
    if (phase != Phase::Searching) return;
    static ULONGLONG next = 0;
    static int tries = 0;
    ULONGLONG now = GetTickCount64();
    if (now < next) return;
    next = now + 2000;
    if (attach()) {
        phase = Phase::Hooked;
        return;
    }
    if (!runtime() && ++tries >= 5) {
        phase = Phase::Absent;
        summary = "not used by this game";
        logger::info("gameinput: runtime not loaded, the game reads input through window messages");
    } else if (runtime() && ++tries % 15 == 0) {
        logger::info("gameinput: runtime loaded, waiting for a keyboard and a mouse reading");
    }
}

bool active() { return phase == Phase::Hooked; }

const char* status() { return summary.c_str(); }

void hold(int vk) {
    std::scoped_lock g(overlayLock);
    pendingHold.set(vk & 0xFF);
}

void drop(int vk) {
    std::scoped_lock g(overlayLock);
    pendingDrop.set(vk & 0xFF);
}

void scaleMouse(float factor) {
    std::scoped_lock g(overlayLock);
    pendingScale *= factor;
}

void holdWheel() {
    std::scoped_lock g(overlayLock);
    pendingWheel = true;
}

void beginFrame() {
    std::scoped_lock g(overlayLock);
    pendingHold.reset();
    pendingDrop.reset();
    pendingScale = 1.f;
    pendingWheel = false;
}

void publish() {
    std::scoped_lock g(overlayLock);
    activeHold = pendingHold;
    activeDrop = pendingDrop;
    activeScale = pendingScale;
    activeWheel = pendingWheel;
}

}
