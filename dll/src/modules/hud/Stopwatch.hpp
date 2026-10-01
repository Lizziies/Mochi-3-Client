#pragma once

#include "modules/HudModule.hpp"
#include "render/Ui.hpp"

#include <windows.h>

#include <format>

class Stopwatch : public TextHud {
public:
    Stopwatch() : TextHud("Stopwatch", "Stopwatch with its own start/stop and reset keys.", {"hud-self"}, {0.01f, 0.14f}) {
        sub("Timer");
    }

    void onKey(KeyEvent& ev) override {
        if (!ev.down || ev.repeat) return;
        double now = ui::time();
        if (ev.vk == startStop_.i) {
            if (running_) accumulated_ += now - started_;
            else started_ = now;
            running_ = !running_;
        } else if (ev.vk == reset_.i) {
            running_ = false;
            accumulated_ = 0;
        }
    }

protected:
    std::string label() const override { return "Time"; }

    std::string value() override {
        double total = accumulated_ + (running_ ? ui::time() - started_ : 0);
        int ms = int(total * 1000) % 1000;
        int sec = int(total) % 60;
        int min = int(total) / 60;
        return std::format("{:02}:{:02}.{:01}", min, sec, ms / 100);
    }

private:
    Setting& startStop_ = keySetting("startStop", "Start / stop", VK_F6);
    Setting& reset_ = keySetting("reset", "Reset", VK_F7);
    bool running_ = false;
    double started_ = 0;
    double accumulated_ = 0;
};
