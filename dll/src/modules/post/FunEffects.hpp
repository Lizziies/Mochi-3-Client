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
        : Module("Deepfry", "Frittiert das Spielbild: übersättigt, körnig und hart abgestuft. Reiner Spaß.", Category::Fun,
                 {"cosmetic"}) {
        sub("Post-Effekte");
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
    Setting& amount_ = slider("amount", "Stärke", 0.5f, 0.05f, 1.f, "%.2f");
    Setting& mode_ = choice("mode", "Verlauf", {"Dauerhaft", "Pulsierend", "Bei Klick"});
    Setting& speed_ = slider("speed", "Puls-Tempo (Hz)", 0.6f, 0.1f, 4.f, "%.2f");
    float phase_ = 0.f;
    float pulse_ = 0.f;
};

class UpsideDown : public Module {
public:
    UpsideDown()
        : Module("Upside Down", "Dreht das Spielbild auf den Kopf. Die Steuerung bleibt normal, das ist Absicht.", Category::Fun,
                 {"cosmetic"}) {
        sub("Post-Effekte");
        hold_.visible = [this] { return holdMode_.b; };
    }

    void onFrame() override {
        if (holdMode_.b && hold_.i && !input::down(hold_.i)) return;
        post::params().flip = 1.f;
    }

private:
    Setting& holdMode_ = toggleSetting("holdMode", "Nur solange eine Taste gehalten wird", false);
    Setting& hold_ = keySetting("hold", "Halte-Taste", VK_F8);
};
