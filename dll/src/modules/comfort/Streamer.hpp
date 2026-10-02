#pragma once

#include "gui/Notify.hpp"
#include "modules/Manager.hpp"
#include "modules/Module.hpp"

#include <sstream>

class StreamerMode : public Module {
public:
    StreamerMode()
        : Module("Streamer Mode", "Hides IP, coordinates, server and chat on a key. A second press brings everything back.",
                 Category::Comfort, {"cosmetic"}) {
        sub("Profiles");
    }

    void onKey(KeyEvent& ev) override {
        if (!ev.down || ev.repeat || !hotkey_.i || ev.vk != hotkey_.i) return;
        if (active_) restore();
        else hide();
    }

    void onDisable() override {
        if (active_) restore();
    }

private:
    void hide() {
        std::stringstream ss(list_.text);
        std::string name;
        while (std::getline(ss, name, ',')) {
            auto a = name.find_first_not_of(' '), b = name.find_last_not_of(' ');
            if (a == std::string::npos) continue;
            name = name.substr(a, b - a + 1);
            auto* m = modules::find(name);
            if (!m || m == this || !m->userEnabled()) continue;
            m->setEnabled(false);
            hidden_.push_back(name);
        }
        active_ = true;
        if (toast_.b) notify::push("Streamer Mode", i18n::tr("Sensitive displays are off."), notify::Kind::Info);
    }

    void restore() {
        for (auto& name : hidden_)
            if (auto* m = modules::find(name)) m->setEnabled(true);
        hidden_.clear();
        active_ = false;
        if (toast_.b) notify::push("Streamer Mode", i18n::tr("Displays are back on."), notify::Kind::Info);
    }

    Setting& hotkey_ = keySetting("hotkey", "Toggle", 0);
    Setting& list_ = textSetting("list", "Modules that get hidden (comma)", "IP Display, Coordinates, Server Display, Waypoints, Better Chat, Death Logger");
    Setting& toast_ = toggleSetting("toast", "Show notice", true);
    std::vector<std::string> hidden_;
    bool active_ = false;
};
