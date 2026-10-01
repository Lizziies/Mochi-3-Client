#pragma once

#include "hook/Dx.hpp"
#include "hook/Input.hpp"
#include "modules/Module.hpp"
#include "modules/perf/Tuning.hpp"

#include <windows.h>

#include <algorithm>

class InstantInput : public Module {
public:
    InstantInput()
        : Module("Instant Hit",
                 "Shortens the local chain from click to frame. Creates no clicks. Banned on some servers.",
                 Category::Pvp, {"input", "timing"}) {
        sub("Input");
        markRisky();
        tearing_.visible = [this] { return queue_.b; };
    }

    void onDisable() override { restore(); }

    void onFrame() override {
        if (queue_.b) perf::lowLatency();
        if (queue_.b && tearing_.b) perf::tearing();
        if (priority_.b) boost();
        else restore();
        trackClicks();
    }

    void drawSettings() override {
        ImGui::Spacing();
        if (count_) ImGui::TextDisabled(i18n::tr("Click to frame: %.1f ms on average (%d clicks)"), sum_ / count_, count_);
        else ImGui::TextDisabled(i18n::tr("Click to measure the latency."));
        if (ImGui::SmallButton(i18n::tr("Reset measurement"))) sum_ = 0, count_ = 0;
    }

private:
    void boost() {
        if (thread_) return;
        thread_ = OpenThread(THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION, FALSE, GetCurrentThreadId());
        if (!thread_) return;
        base_ = GetThreadPriority(thread_);
        SetThreadPriority(thread_, std::max(base_, (int)THREAD_PRIORITY_ABOVE_NORMAL));
        SetThreadPriorityBoost(thread_, TRUE);
    }

    void restore() {
        if (!thread_) return;
        SetThreadPriority(thread_, base_);
        SetThreadPriorityBoost(thread_, FALSE);
        CloseHandle(thread_);
        thread_ = nullptr;
    }

    void trackClicks() {
        auto& fi = dx::frame();
        int64_t click = input::lastClickQpc();
        if (!click || click == seen_ || fi.presentQpc <= click) return;
        seen_ = click;
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        double ms = double(fi.presentQpc - click) * 1000.0 / double(f.QuadPart);
        if (ms < 250) {
            sum_ += (float)ms;
            count_++;
        }
    }

    Setting& queue_ = toggleSetting("queue", "Short frame queue", true);
    Setting& tearing_ = toggleSetting("tearing", "Allow tearing (VSync off)", false);
    Setting& priority_ = toggleSetting("priority", "Render thread with higher priority", true);
    HANDLE thread_ = nullptr;
    int base_ = 0;
    int64_t seen_ = 0;
    float sum_ = 0.f;
    int count_ = 0;
};
