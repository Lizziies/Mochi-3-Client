#include "Tuning.hpp"
#include "hook/Dx.hpp"

namespace perf {

static bool queue = false;
static bool tear = false;
static float cap = 0.f;

void begin() {
    queue = false;
    tear = false;
    cap = 0.f;
}

void apply() {
    auto& t = dx::tuning();
    t.lowLatency = queue;
    t.allowTearing = tear;
    t.fpsLimit = cap;
}

void lowLatency() { queue = true; }

void tearing() { tear = true; }

void limit(float fps) {
    if (fps < 10.f) return;
    if (cap == 0.f || fps < cap) cap = fps;
}

}
