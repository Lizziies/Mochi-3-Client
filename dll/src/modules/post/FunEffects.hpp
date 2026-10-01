#pragma once

#include "PostFx.hpp"
#include "hook/Input.hpp"
#include "modules/Module.hpp"
#include "render/Ui.hpp"

#include <windows.h>

#include <algorithm>
#include <cmath>

class Deepfry : public Module {
public:
    Deepfry()
        : Module("Deepfry", "Deep-fries the game image: oversaturated, grainy and harshly stepped. Pure fun.", Category::Fun,
                 {"cosmetic"}) {
        sub("Post effects");
        speed_.visible = [this] { return mode_.i == 1; };
    }

    void onFrame() override {
        float k = amount_.f;
        if (mode_.i == 1) {
            phase_ = std::fmod(phase_ + ui::dt() * speed_.f * 6.2832f, 6.2832f);
            k *= 0.5f + 0.5f * std::sin(phase_);
        } else if (mode_.i == 2) {
            pulse_ = std::max(input::down(VK_LBUTTON) ? 1.f : 0.f, pulse_ - ui::dt() * 4.f);
            k *= pulse_;
        }
        post::params().fry = std::max(post::params().fry, std::clamp(k, 0.f, 1.f));
    }

private:
    Setting& amount_ = slider("amount", "Strength", 0.5f, 0.05f, 1.f, "%.2f");
    Setting& mode_ = choice("mode", "Gradient", {"Permanent", "Pulsing", "On click"});
    Setting& speed_ = slider("speed", "Pulse speed (Hz)", 0.6f, 0.1f, 4.f, "%.2f");
    float phase_ = 0.f;
    float pulse_ = 0.f;
};

class UpsideDown : public Module {
public:
    UpsideDown()
        : Module("Upside Down", "Turns the game image upside down. Controls stay normal, on purpose.", Category::Fun,
                 {"cosmetic"}) {
        sub("Post effects");
        hold_.visible = [this] { return holdMode_.b; };
    }

    void onFrame() override {
        if (holdMode_.b && hold_.i && !input::down(hold_.i)) return;
        post::params().flip = 1.f;
    }

private:
    Setting& holdMode_ = toggleSetting("holdMode", "Only while a key is held", false);
    Setting& hold_ = keySetting("hold", "Hold key", VK_F8);
};
