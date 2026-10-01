#include "SelfTest.hpp"
#include "Manager.hpp"
#include "core/Client.hpp"
#include "core/Log.hpp"
#include "sdk/Game.hpp"

#include <windows.h>

#include <algorithm>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

namespace selftest {

namespace {

constexpr int framesPerModule = 120;

bool on = std::getenv("MOCHI_SELFTEST") != nullptr;
std::mt19937 rng{12345};
size_t index = 0;
int frame = 0;
bool started = false;
std::vector<std::string> failed;
std::vector<std::string> skipped;
int tested = 0;

float rnd(float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); }

void mutate(Module& m) {
    auto& all = m.settings();
    if (all.empty()) return;
    Setting& s = all[std::uniform_int_distribution<size_t>(0, all.size() - 1)(rng)];
    if (s.id == "key" || s.id == "hold") return;
    switch (s.type) {
    case SettingType::Bool: s.b = !s.b; break;
    case SettingType::Float: {
        int pick = std::uniform_int_distribution<int>(0, 3)(rng);
        s.f = pick == 0 ? s.fmin : pick == 1 ? s.fmax : rnd(s.fmin, s.fmax);
        break;
    }
    case SettingType::Int: {
        int pick = std::uniform_int_distribution<int>(0, 3)(rng);
        s.i = pick == 0 ? s.imin : pick == 1 ? s.imax : std::uniform_int_distribution<int>(s.imin, s.imax)(rng);
        break;
    }
    case SettingType::Color: s.color = {rnd(0, 1), rnd(0, 1), rnd(0, 1), rnd(0, 1)}; break;
    case SettingType::Choice:
        if (!s.choices.empty()) s.i = std::uniform_int_distribution<int>(0, (int)s.choices.size() - 1)(rng);
        break;
    case SettingType::Text: s.text = std::string(std::uniform_int_distribution<int>(0, 40)(rng), 'x'); break;
    default: break;
    }
}

void poke(Module& m) {
    static const int keys[] = {'W', 'A', 'S', 'D', VK_SPACE, VK_SHIFT, VK_CONTROL, 'E', VK_ESCAPE, VK_TAB, VK_F5};
    KeyEvent k;
    k.vk = keys[std::uniform_int_distribution<size_t>(0, std::size(keys) - 1)(rng)];
    k.down = std::uniform_int_distribution<int>(0, 1)(rng);
    modules::dispatchKey(k);

    MouseEvent e;
    e.button = std::uniform_int_distribution<int>(0, 1) (rng) ? MouseButton::Left : MouseButton::Right;
    e.down = std::uniform_int_distribution<int>(0, 1)(rng);
    e.wheel = std::uniform_int_distribution<int>(-2, 2)(rng);
    e.dx = std::uniform_int_distribution<int>(-300, 300)(rng);
    e.dy = std::uniform_int_distribution<int>(-300, 300)(rng);
    modules::dispatchMouse(e);
}

void finish() {
    logger::info("selftest: tested {} modules, {} skipped (locked), {} failed", tested, skipped.size(), failed.size());
    for (auto& n : failed) logger::error("selftest FAIL: {}", n);
    logger::info("selftest done");
    client::requestUnload();
}

}

bool active() { return on; }

void tick() {
    if (!on) return;
    auto& list = modules::all();

    if (!started) {
        started = true;
        if (auto* support = modules::find("Game Support"))
            for (auto& st : support->settings())
                if (st.id == "demo") st.b = true;
        game::setDemo(true);
        modules::refreshSigs();
        logger::info("selftest start: {} modules", list.size());
        return;
    }
    if (index >= list.size()) {
        if (index == list.size()) {
            index++;
            finish();
        }
        return;
    }

    Module& m = *list[index];
    if (m.name() == "ClickGUI" || m.name() == "Game Support" || !m.available()) {
        if (frame == 0 && !m.available()) {
            skipped.push_back(m.name());
            std::string miss;
            for (auto& n : m.missingSigs()) miss += n + " ";
            logger::info("selftest locked: {} needs {}", m.name(), miss);
        }
        index++;
        frame = 0;
        return;
    }

    if (frame == 0) m.setEnabled(true);
    if (frame % 7 == 3) mutate(m);
    if (frame % 5 == 1) poke(m);

    if (++frame >= framesPerModule) {
        tested++;
        if (!m.userEnabled()) {
            failed.push_back(m.name());
            logger::error("selftest: {} was disabled by an error", m.name());
        }
        m.setEnabled(false);
        index++;
        frame = 0;
    }
}

}
