#pragma once

#include "modules/Module.hpp"

#include <windows.h>

#include <deque>

class CpsLimiter : public Module {
public:
    CpsLimiter()
        : Module("CPS Limiter", "Limits your clicks per second, for example for servers with a CPS limit.", Category::Pvp,
                 {"input"}) {
        sub("Eingabe");
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        qpf_ = f.QuadPart;
    }

    void onMouse(MouseEvent& ev) override {
        if (!ev.down) return;
        auto& q = ev.button == MouseButton::Left ? left_ : ev.button == MouseButton::Right ? right_ : other_;
        if (&q == &other_) return;
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
    Setting& leftLimit_ = intSlider("left", "Left max CPS", 16, 0, 30);
    Setting& rightLimit_ = intSlider("right", "Right max CPS", 0, 0, 30);
    std::deque<int64_t> left_, right_, other_;
    int64_t qpf_ = 1;
};
