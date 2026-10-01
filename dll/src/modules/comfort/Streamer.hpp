#pragma once

#include "gui/Notify.hpp"
#include "modules/Manager.hpp"
#include "modules/Module.hpp"

#include <sstream>

class StreamerMode : public Module {
public:
    StreamerMode()
        : Module("Streamer Mode", "Versteckt per Taste IP, Koordinaten, Server und Chat. Zweiter Druck bringt alles zurück.",
                 Category::Comfort, {"cosmetic"}) {
        sub("Profile");
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
        if (toast_.b) notify::push("Streamer Mode", "Sensible Anzeigen sind aus.", notify::Kind::Info);
    }

    void restore() {
        for (auto& name : hidden_)
            if (auto* m = modules::find(name)) m->setEnabled(true);
        hidden_.clear();
        active_ = false;
        if (toast_.b) notify::push("Streamer Mode", "Anzeigen sind wieder an.", notify::Kind::Info);
    }

    Setting& hotkey_ = keySetting("hotkey", "Umschalten", 0);
    Setting& list_ = textSetting("list", "Module, die versteckt werden (Komma)", "IP Display, Coordinates, Server Display, Waypoints, Chat Plus, Death Logger");
    Setting& toast_ = toggleSetting("toast", "Hinweis anzeigen", true);
    std::vector<std::string> hidden_;
    bool active_ = false;
};
