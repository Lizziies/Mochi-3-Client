#include "I18n.hpp"
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
#include "input/NoScroll.hpp"
#include "perf/LowLatency.hpp"
#include "perf/SystemBoost.hpp"
#include "visual/Crosshair.hpp"

#include <windows.h>

namespace modules {

static std::vector<std::unique_ptr<Module>> list;
static bool hudHidden = false;

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

    add<Crosshair>();

    add<CpsLimiter>();
    add<NoScroll>();

    add<LowLatency>();
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
}

const std::vector<std::unique_ptr<Module>>& all() { return list; }

Module* find(const std::string& name) {
    for (auto& m : list)
        if (m->name() == name) return m.get();
    return nullptr;
}

static void fault(Module& m) {
    m.setEnabled(false);
    notify::push(i18n::tr("Module disabled"), i18n::fmt("{} had an error. See the log for details.", m.name()), notify::Kind::Error);
}

void frame(ImDrawList* hud) {
    if (sigs::takeChanged()) refreshSigs();
    rules::tick();

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
}

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
                notify::push(m->name(), i18n::tr("Not allowed on this server."), notify::Kind::Warn);
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
