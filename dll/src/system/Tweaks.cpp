#include "Tweaks.hpp"
#include "core/Log.hpp"

#include <windows.h>
#include <timeapi.h>

namespace tweaks {

static bool timer = false;
static bool priority = false;
static bool throttling = false;
static DWORD originalPriority = NORMAL_PRIORITY_CLASS;

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
    timerResolution(false);
    highPriority(false);
    noPowerThrottling(false);
}

}
