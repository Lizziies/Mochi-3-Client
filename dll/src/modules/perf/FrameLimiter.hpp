#pragma once

#include "Tuning.hpp"
#include "gui/Gui.hpp"
#include "hook/Dx.hpp"
#include "modules/Module.hpp"

#include <windows.h>

#include <algorithm>

class FrameLimiter : public Module {
public:
    FrameLimiter()
        : Module("Frame Limiter",
                 "Precise FPS limiter, throttles in the background and in the menu.",
                 Category::Performance, {"performance"}) {
        sub("Frame timing");
        fps_.visible = [this] { return mode_.i == 0; };
        offset_.visible = [this] { return mode_.i == 1; };
        bgFps_.visible = [this] { return background_.b; };
        menuFps_.visible = [this] { return menu_.b; };
    }

    void onFrame() override {
        float base = baseLimit();
        float cap = base;
        if (background_.b && !focused()) cap = tighter(cap, bgFps_.f);
        if (menu_.b && gui::open()) cap = tighter(cap, menuFps_.f);
        effective_ = cap;
        perf::limit(cap);
    }

    void drawSettings() override {
        ImGui::Spacing();
        if (effective_ > 0.f) ImGui::TextDisabled(i18n::tr("Active limit: %.0f FPS  ·  Monitor: %d Hz"), effective_, refresh());
        else ImGui::TextDisabled(i18n::tr("No limit active  ·  Monitor: %d Hz"), refresh());
        float ms = (float)dx::frame().frameMs;
        if (ms > 0.f) ImGui::TextDisabled(i18n::tr("Last frame: %.2f ms (%.0f FPS)"), ms, 1000.f / ms);
    }

private:
    static float tighter(float cap, float limit) { return cap <= 0.f ? limit : std::min(cap, limit); }

    float baseLimit() const {
        if (mode_.i == 0) return fps_.f;
        if (mode_.i == 1) return std::max(10.f, float(refresh()) - offset_.f);
        return 0.f;
    }

    static bool focused() {
        HWND w = dx::window();
        return !w || GetForegroundWindow() == w;
    }

    // asking the driver every frame costs up to milliseconds, the rate rarely changes
    static int refresh() {
        static int cached = 60;
        static ULONGLONG at = 0;
        ULONGLONG now = GetTickCount64();
        if (at && now - at < 2000) return cached;
        at = now;
        cached = query();
        return cached;
    }

    static int query() {
        HWND w = dx::window();
        MONITORINFOEXW mi{};
        mi.cbSize = sizeof(mi);
        DEVMODEW dm{};
        dm.dmSize = sizeof(dm);
        HMONITOR mon = MonitorFromWindow(w ? w : GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
        if (GetMonitorInfoW(mon, &mi) && EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm) && dm.dmDisplayFrequency > 1)
            return (int)dm.dmDisplayFrequency;
        return 60;
    }

    Setting& mode_ = choice("mode", "Limit", {"Fixed value", "Refresh rate minus offset", "Off"}, 1);
    Setting& fps_ = slider("fps", "FPS limit", 240.f, 30.f, 1000.f, "%.0f");
    Setting& offset_ = slider("offset", "Offset (FPS)", 3.f, 0.f, 20.f, "%.0f");
    Setting& background_ = toggleSetting("background", "Throttle in the background", true);
    Setting& bgFps_ = slider("bgFps", "Background limit", 30.f, 5.f, 120.f, "%.0f");
    Setting& menu_ = toggleSetting("menu", "Throttle while the menu is open", false);
    Setting& menuFps_ = slider("menuFps", "Menu limit", 90.f, 30.f, 240.f, "%.0f");
    float effective_ = 0.f;
};
