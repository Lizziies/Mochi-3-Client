#pragma once

#include "modules/HudModule.hpp"
#include "render/Ui.hpp"

#include <windows.h>
#include <psapi.h>

#include <format>

class Memory : public TextHud {
public:
    Memory() : TextHud("Memory", "Shows how much RAM Minecraft is using.", {"hud-self"}, {0.005f, 0.21f}) {
        sub("Diagnostics");
    }

protected:
    std::string label() const override { return "RAM"; }

    std::string value() override {
        double now = ui::time();
        if (now - last_ > 1.0) {
            last_ = now;
            PROCESS_MEMORY_COUNTERS pmc{};
            if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) mb_ = pmc.WorkingSetSize / (1024.0 * 1024.0);
            MEMORYSTATUSEX ms{sizeof(ms)};
            if (GlobalMemoryStatusEx(&ms)) load_ = ms.dwMemoryLoad;
        }
        if (mode_.i == 1) return i18n::fmt("{}% system", load_);
        return std::format("{:.0f} MB", mb_);
    }

private:
    Setting& mode_ = choice("mode", "Display", {"Minecraft (MB)", "System (%)"});
    double last_ = -10;
    double mb_ = 0;
    unsigned load_ = 0;
};
