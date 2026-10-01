#include "Tweaks.hpp"
#include "core/Log.hpp"

#include <windows.h>
#include <timeapi.h>

#include <atomic>

namespace tweaks {

static bool timer = false;
static bool priority = false;
static bool throttling = false;
static DWORD originalPriority = NORMAL_PRIORITY_CLASS;
static std::atomic<bool> boostWanted{false};

using SetCharsFn = HANDLE(WINAPI*)(LPCWSTR, LPDWORD);
using SetPrioFn = BOOL(WINAPI*)(HANDLE, int);
using RevertFn = BOOL(WINAPI*)(HANDLE);

struct Avrt {
    SetCharsFn set = nullptr;
    SetPrioFn priority = nullptr;
    RevertFn revert = nullptr;

    Avrt() {
        HMODULE lib = LoadLibraryW(L"avrt.dll");
        if (!lib) return;
        set = reinterpret_cast<SetCharsFn>(reinterpret_cast<void*>(GetProcAddress(lib, "AvSetMmThreadCharacteristicsW")));
        priority = reinterpret_cast<SetPrioFn>(reinterpret_cast<void*>(GetProcAddress(lib, "AvSetMmThreadPriority")));
        revert = reinterpret_cast<RevertFn>(reinterpret_cast<void*>(GetProcAddress(lib, "AvRevertMmThreadCharacteristics")));
    }
};

void inputBoost(bool on) { boostWanted = on; }

bool wantsInputBoost() { return boostWanted; }

// priority 2 is AVRT_PRIORITY_HIGH
void threadBoost(bool on) {
    static Avrt avrt;
    thread_local HANDLE task = nullptr;
    if (on == (task != nullptr) || !avrt.set) return;
    if (on) {
        DWORD index = 0;
        task = avrt.set(L"Games", &index);
        if (task && avrt.priority) avrt.priority(task, 2);
    } else {
        avrt.revert(task);
        task = nullptr;
    }
}

void timerResolution(bool on) {
    if (on == timer) return;
    timer = on;
    if (on) timeBeginPeriod(1);
    else timeEndPeriod(1);
}

void highPriority(bool on) {
    if (on == priority) return;
    priority = on;
    HANDLE self = GetCurrentProcess();
    if (on) {
        originalPriority = GetPriorityClass(self);
        SetPriorityClass(self, ABOVE_NORMAL_PRIORITY_CLASS);
    } else {
        SetPriorityClass(self, originalPriority);
    }
}

void noPowerThrottling(bool on) {
    if (on == throttling) return;
    throttling = on;
    PROCESS_POWER_THROTTLING_STATE state{};
    state.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    state.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
    state.StateMask = 0;
    if (!on) state.ControlMask = 0;
    if (!SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &state, sizeof(state)))
        logger::warn("power throttling change failed: {}", GetLastError());
}

void restore() {
    boostWanted = false;
    threadBoost(false);
    timerResolution(false);
    highPriority(false);
    noPowerThrottling(false);
}

}
