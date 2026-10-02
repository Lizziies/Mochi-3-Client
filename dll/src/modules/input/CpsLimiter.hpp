#pragma once

#include "modules/Module.hpp"

#include <windows.h>

#include <deque>

class CpsLimiter : public Module {
public:
    CpsLimiter()
        : Module("CPS Limiter", "Limits your clicks per second, for example for servers with a CPS limit. Can also filter the double clicks of a worn mouse switch.", Category::Pvp,
                 {"input"}) {
        sub("Input");
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        qpf_ = f.QuadPart;
    }

    void onMouse(MouseEvent& ev) override {
        int b = ev.button == MouseButton::Left ? 0 : ev.button == MouseButton::Right ? 1 : -1;
        if (b < 0) return;
        if (bounced(b, ev)) return;
        if (!ev.down) return;
        auto& q = b == 0 ? left_ : right_;
        int limit = ev.button == MouseButton::Left ? leftLimit_.i : rightLimit_.i;
        if (limit <= 0) return;
        while (!q.empty() && ev.qpc - q.front() > qpf_) q.pop_front();
        if ((int)q.size() >= limit) {
            ev.cancel = true;
            return;
        }
        q.push_back(ev.qpc);
    }

private:
    // A worn switch bounces: the button comes up and goes down again within a few milliseconds. Such a press is
    // dropped together with its release, so the game never sees half a click.
    bool bounced(int b, MouseEvent& ev) {
        if (!ev.down) {
            if (swallow_[b]) {
                swallow_[b] = false;
                ev.cancel = true;
                return true;
            }
            lastUp_[b] = ev.qpc;
            return false;
        }
        if (debounce_.f <= 0.f || !lastUp_[b] || ev.qpc - lastUp_[b] > int64_t(debounce_.f * 0.001 * double(qpf_))) return false;
        swallow_[b] = true;
        ev.cancel = true;
        return true;
    }

    Setting& leftLimit_ = intSlider("left", "Left max CPS", 16, 0, 30);
    Setting& rightLimit_ = intSlider("right", "Right max CPS", 0, 0, 30);
    Setting& debounce_ = slider("debounce", "Double click fix (ms)", 0.f, 0.f, 80.f, "%.0f ms");
    std::deque<int64_t> left_, right_;
    int64_t lastUp_[2] = {};
    bool swallow_[2] = {};
    int64_t qpf_ = 1;
};
