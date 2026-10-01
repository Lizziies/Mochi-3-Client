#include "Manager.hpp"
#include "core/Guard.hpp"
#include "core/Log.hpp"
#include "gui/Gui.hpp"
#include "gui/Notify.hpp"
#include "gui/Widgets.hpp"
#include "server/Rules.hpp"
#include "sig/Sigs.hpp"

#include "client/ClickGui.hpp"
#include "fun/DvdScreen.hpp"
#include "fun/EyeBreak.hpp"
#include "fun/Flappy.hpp"
#include "fun/Snake.hpp"
#include "hud/Clock.hpp"
#include "hud/Cps.hpp"
#include "hud/Fps.hpp"
#include "hud/Keystrokes.hpp"
#include "hud/Latency.hpp"
#include "hud/Memory.hpp"
#include "hud/MouseStrokes.hpp"
#include "hud/ServerInfo.hpp"
#include "hud/SessionTimer.hpp"
#include "hud/Stopwatch.hpp"
#include "input/CpsLimiter.hpp"
#include "input/InstantInput.hpp"
#include "input/NoScroll.hpp"
#include "network/LatencyBlame.hpp"
#include "network/Network.hpp"
#include "network/PingCounter.hpp"
#include "network/Probe.hpp"
#include "perf/FrameLimiter.hpp"
#include "perf/LowLatency.hpp"
#include "perf/Tuning.hpp"
#include "perf/SystemBoost.hpp"
#include "visual/Crosshair.hpp"

#include <windows.h>

namespace modules {

static std::vector<std::unique_ptr<Module>> list;
static bool hudHidden = false;
static float cost = 0.f;

template <class T>
static void add() {
    list.push_back(std::make_unique<T>());
}

void init() {
    add<ClickGui>();

    add<Fps>();
    add<Cps>();
    add<Keystrokes>();
    add<MouseStrokes>();
    add<Clock>();
    add<Stopwatch>();
    add<SessionTimer>();
    add<Memory>();
    add<LatencyHud>();
    add<ServerInfo>();
    add<PingCounter>();
    add<Network>();
    add<LatencyBlame>();

    add<Crosshair>();

    add<CpsLimiter>();
    add<NoScroll>();
    add<InstantInput>();

    add<LowLatency>();
    add<FrameLimiter>();
    add<SystemBoost>();

    add<Snake>();
    add<Flappy>();
    add<DvdScreen>();
    add<EyeBreak>();

    refreshSigs();
    for (auto& m : list)
        if (m->alwaysOn()) m->setEnabled(true);

    logger::info("{} modules registered", list.size());
}

void shutdown() {
    for (auto& m : list) {
        if (m->enabled()) guard::call(m->name().c_str(), [&] { m->onDisable(); });
    }
    probe::shutdown();
}

const std::vector<std::unique_ptr<Module>>& all() { return list; }

Module* find(const std::string& name) {
    for (auto& m : list)
        if (m->name() == name) return m.get();
    return nullptr;
}

static void fault(Module& m) {
    m.setEnabled(false);
    notify::push("Modul abgeschaltet", m.name() + " hatte einen Fehler. Details im Log.", notify::Kind::Error);
}

void frame(ImDrawList* hud) {
    if (sigs::takeChanged()) refreshSigs();
    rules::tick();

    LARGE_INTEGER t0, t1, qpf;
    QueryPerformanceCounter(&t0);
    perf::begin();
    bool editing = gui::editingHud();
    for (auto& m : list) {
        if (!m->enabled()) continue;
        if (!guard::call(m->name().c_str(), [&] { m->onFrame(); })) {
            fault(*m);
            continue;
        }
        if (m->isHud() && (hudHidden && !editing)) continue;
        if (!guard::call(m->name().c_str(), [&] { m->onRender(hud); })) fault(*m);
    }
    perf::apply();
    QueryPerformanceCounter(&t1);
    QueryPerformanceFrequency(&qpf);
    cost += (float(double(t1.QuadPart - t0.QuadPart) * 1000.0 / double(qpf.QuadPart)) - cost) * 0.05f;
}

float costMs() { return cost; }

void dispatchKey(KeyEvent& ev) {
    if (widgets::capturingKey()) return;
    if (ev.down && !ev.repeat && ev.vk == VK_F1) hudHidden = !hudHidden;

    bool captured = gui::capturesKeyboard();
    if (ev.down && !ev.repeat && !captured) {
        for (auto& m : list) {
            int key = m->keybind().i;
            if (!key || key != ev.vk || m->alwaysOn()) continue;
            m->toggle();
            if (m->rule() == RuleLevel::Block)
                notify::push(m->name(), "Auf diesem Server nicht erlaubt.", notify::Kind::Warn);
        }
    }

    for (auto& m : list) {
        if (!m->enabled() || (captured && !m->alwaysOn())) continue;
        if (!guard::call(m->name().c_str(), [&] { m->onKey(ev); })) fault(*m);
        if (ev.cancel) return;
    }
}

void dispatchMouse(MouseEvent& ev) {
    for (auto& m : list) {
        if (!m->enabled()) continue;
        if (!guard::call(m->name().c_str(), [&] { m->onMouse(ev); })) fault(*m);
        if (ev.cancel) return;
    }
}

void dispatchServer(const ServerEvent& ev) {
    for (auto& m : list) {
        if (!m->enabled()) continue;
        guard::call(m->name().c_str(), [&] { m->onServer(ev); });
    }
}

void refreshSigs() {
    for (auto& m : list) {
        m->checkSigs();
        m->setEnabled(m->userEnabled());
    }
}

}
