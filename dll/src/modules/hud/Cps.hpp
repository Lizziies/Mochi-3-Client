#pragma once

#include "hook/Input.hpp"
#include "modules/HudModule.hpp"

#include <format>

class Cps : public TextHud {
public:
    bool defaultEnabled() const override { return true; }

    Cps() : TextHud("CPS", "Counts your clicks per second.", {"hud-self"}, {0.01f, 0.06f}) {
        sub("Eigene Werte");
    }

protected:
    std::string label() const override { return "CPS"; }

    std::string value() override {
        int l = input::cps(MouseButton::Left), r = input::cps(MouseButton::Right);
        switch (mode_.i) {
        case 1: return std::to_string(l);
        case 2: return std::to_string(r);
        default: return std::format("{} | {}", l, r);
        }
    }

private:
    Setting& mode_ = choice("mode", "Display", {"Left | Right", "Left only", "Right only"});
};
