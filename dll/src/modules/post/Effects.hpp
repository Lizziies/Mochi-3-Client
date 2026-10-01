#pragma once

#include "PostFx.hpp"
#include "modules/Manager.hpp"
#include "modules/Module.hpp"
#include "render/Ui.hpp"

#include <windows.h>

#include <algorithm>
#include <cmath>

class SaturationHue : public Module {
public:
    SaturationHue()
        : Module("Saturation / Hue", "Ändert Sättigung und Farbton des Spielbilds, mit Vorlagen und optionalem Regenbogen-Wechsel.",
                 Category::Visual, {"cosmetic"}) {
        sub("Post-Effekte");
        saturation_.visible = [this] { return preset_.i == 0; };
        hue_.visible = [this] { return preset_.i == 0 && !cycle_.b; };
        speed_.visible = [this] { return cycle_.b; };
    }

    void onFrame() override {
        auto& p = post::params();
        float sat = saturation_.f, hue = hue_.f;
        switch (preset_.i) {
        case 1: sat = 0.f; break;
        case 2: sat = 1.45f; break;
        case 3: sat = 0.75f; p.brightness += 0.04f; break;
        case 4: sat = 1.15f; hue = -12.f; break;
        case 5: sat = 0.55f; hue = 18.f; break;
        default: break;
        }
        if (cycle_.b) {
            phase_ = std::fmod(phase_ + ui::dt() * speed_.f * 36.f, 360.f);
            hue = phase_ - 180.f;
        }
        p.saturation *= sat;
        p.hue += hue;
    }

private:
    Setting& preset_ = choice("preset", "Vorlage", {"Eigene Werte", "Schwarzweiß", "Lebendig", "Pastell", "Warm", "Retro"});
    Setting& saturation_ = slider("saturation", "Sättigung", 1.2f, 0.f, 2.5f, "%.2f");
    Setting& hue_ = slider("hue", "Farbton", 0.f, -180.f, 180.f, "%.0f°");
    Setting& cycle_ = toggleSetting("cycle", "Farbton durchlaufen", false);
    Setting& speed_ = slider("speed", "Tempo", 0.5f, 0.05f, 3.f, "%.2f");
    float phase_ = 0.f;
};

class BrightnessContrast : public Module {
public:
    BrightnessContrast()
        : Module("Brightness / Contrast", "Helligkeit, Kontrast, Gamma und Vignette für das Spielbild.",
                 Category::Visual, {"cosmetic"}) {
        sub("Post-Effekte");
    }

    void onFrame() override {
        auto& p = post::params();
        p.brightness += brightness_.f;
        p.contrast *= contrast_.f;
        p.gamma *= gamma_.f;
        p.vignette = std::max(p.vignette, vignette_.f);
    }

private:
    Setting& brightness_ = slider("brightness", "Helligkeit", 0.05f, -0.5f, 0.5f, "%.2f");
    Setting& contrast_ = slider("contrast", "Kontrast", 1.1f, 0.5f, 2.f, "%.2f");
    Setting& gamma_ = slider("gamma", "Gamma", 1.f, 0.5f, 2.5f, "%.2f");
    Setting& vignette_ = slider("vignette", "Vignette", 0.f, 0.f, 1.f, "%.2f");
};

class ScreenTint : public Module {
public:
    ScreenTint()
        : Module("Screen Tint", "Legt einen Farbfilter über das Spielbild, zum Beispiel zartes Rosa.",
                 Category::Visual, {"cosmetic"}) {
        sub("Post-Effekte");
        pulseSpeed_.visible = [this] { return pulse_.b; };
        pulseDepth_.visible = [this] { return pulse_.b; };
    }

    void onFrame() override {
        float k = strength_.f;
        if (pulse_.b) {
            phase_ = std::fmod(phase_ + ui::dt() * pulseSpeed_.f * 6.2832f, 6.2832f);
            k *= 1.f - pulseDepth_.f * (0.5f + 0.5f * std::sin(phase_));
        }
        auto& p = post::params();
        p.tint[0] = color_.color.x;
        p.tint[1] = color_.color.y;
        p.tint[2] = color_.color.z;
        p.tint[3] = std::clamp(k * color_.color.w, 0.f, 1.f);
        p.tintMode = mode_.i;
    }

private:
    Setting& color_ = colorSetting("color", "Farbe", {1.f, 0.55f, 0.75f, 1.f});
    Setting& strength_ = slider("strength", "Stärke", 0.14f, 0.f, 1.f, "%.2f");
    Setting& mode_ = choice("mode", "Mischmodus", {"Mischen", "Multiplizieren", "Addieren", "Überlagern"});
    Setting& pulse_ = toggleSetting("pulse", "Pulsieren", false);
    Setting& pulseSpeed_ = slider("pulseSpeed", "Puls-Tempo (Hz)", 0.4f, 0.05f, 3.f, "%.2f");
    Setting& pulseDepth_ = slider("pulseDepth", "Puls-Tiefe", 0.5f, 0.f, 1.f, "%.2f");
    float phase_ = 0.f;
};

class Sharpen : public Module {
public:
    Sharpen()
        : Module("Sharpen", "Schärft das Spielbild nach, nützlich bei niedriger Auflösung oder Skalierung.", Category::Visual,
                 {"cosmetic"}) {
        sub("Post-Effekte");
    }

    void onFrame() override { post::params().sharpen += amount_.f; }

private:
    Setting& amount_ = slider("amount", "Stärke", 0.6f, 0.f, 3.f, "%.2f");
};

class DepthOfField : public Module {
public:
    DepthOfField()
        : Module("Depth of Field", "Weicher Fokus: das Bild wird zum Rand hin unscharf, wie bei einer Kamera mit Tilt-Shift-Effekt.",
                 Category::Visual, {"cosmetic"}) {
        sub("Post-Effekte");
    }

    void onFrame() override { post::params().dof = std::max(post::params().dof, amount_.f); }

private:
    Setting& amount_ = slider("amount", "Stärke", 0.5f, 0.f, 1.f, "%.2f");
};

class ColorFilter : public Module {
public:
    ColorFilter()
        : Module("Color Filter", "Farbfilter für Farbsehschwäche: korrigiert oder simuliert Rot-, Grün- und Blauschwäche.",
                 Category::Visual, {"cosmetic"}) {
        sub("Post-Effekte");
    }

    void onFrame() override {
        int base = type_.i + 1;
        post::params().colorMode = base + (simulate_.b ? 3 : 0);
    }

private:
    Setting& type_ = choice("type", "Art", {"Rotschwäche (Protanopie)", "Grünschwäche (Deuteranopie)", "Blauschwäche (Tritanopie)"});
    Setting& simulate_ = toggleSetting("simulate", "Nur simulieren statt korrigieren", false);
};

class NightShift : public Module {
public:
    NightShift()
        : Module("Night Shift", "Wärmerer Blaufilter für entspanntere Augen, auf Wunsch automatisch am Abend.", Category::Visual,
                 {"cosmetic"}) {
        sub("Post-Effekte");
        from_.visible = [this] { return auto_.b; };
        to_.visible = [this] { return auto_.b; };
    }

    void onFrame() override {
        if (auto_.b && !inWindow()) return;
        float k = temperature_.f;
        float r = 1.f, g, b;
        float t = k / 100.f;
        g = std::clamp(t <= 66.f ? 0.39008657f * std::log(t) - 0.63184144f : 1.29293619f * std::pow(t - 60.f, -0.1332047592f), 0.f, 1.f);
        b = t >= 66.f ? 1.f : (t <= 19.f ? 0.f : std::clamp(0.54320679f * std::log(t - 10.f) - 1.19625409f, 0.f, 1.f));
        auto& p = post::params();
        p.night[0] = r;
        p.night[1] = g;
        p.night[2] = b;
        p.night[3] = strength_.f;
    }

private:
    bool inWindow() const {
        SYSTEMTIME st;
        GetLocalTime(&st);
        float now = st.wHour + st.wMinute / 60.f;
        float a = from_.f, e = to_.f;
        return a <= e ? now >= a && now < e : now >= a || now < e;
    }

    Setting& temperature_ = slider("temperature", "Farbtemperatur (K)", 3400.f, 1500.f, 6500.f, "%.0f");
    Setting& strength_ = slider("strength", "Stärke", 0.8f, 0.f, 1.f, "%.2f");
    Setting& auto_ = toggleSetting("auto", "Nur zu bestimmter Uhrzeit", false);
    Setting& from_ = slider("from", "Ab (Uhr)", 20.f, 0.f, 24.f, "%.1f");
    Setting& to_ = slider("to", "Bis (Uhr)", 7.f, 0.f, 24.f, "%.1f");
};

class MotionBlur : public Module {
public:
    MotionBlur()
        : Module("Motion Blur", "Bewegungsunschärfe aus Kamerabewegung und Bildmischung. Stärke, Qualität und Schwelle einstellbar.",
                 Category::Visual, {"cosmetic"}) {
        sub("Post-Effekte");
        samples_.visible = [this] { return mode_.i != 0; };
        only_.visible = [this] { return mode_.i != 1; };
        threshold_.visible = [this] { return mode_.i == 1 || only_.b; };
    }

    void onFrame() override {
        auto d = modules::mouseDelta();
        auto ds = ImGui::GetIO().DisplaySize;
        float speed = std::sqrt(float(d.x * d.x + d.y * d.y));
        motion_ += (speed - motion_) * std::min(1.f, ui::dt() * 14.f);
        float amount = std::clamp(motion_ / std::max(threshold_.f, 1.f), 0.f, 1.f);
        auto& p = post::params();

        if (mode_.i != 1) {
            float k = only_.b ? amount : 1.f;
            p.blend = std::max(p.blend, std::min(0.92f, strength_.f * 0.9f * k));
        }
        if (mode_.i != 0 && speed > 0.f && ds.x > 0.f) {
            float len = std::min(0.08f, motion_ * strength_.f * 1.6f / ds.x);
            float nx = float(d.x) / speed, ny = float(d.y) / speed;
            p.dir[0] = nx * len;
            p.dir[1] = ny * len * (ds.x / std::max(ds.y, 1.f));
            p.dirSamples = samples_.i;
        }
    }

private:
    Setting& mode_ = choice("mode", "Art", {"Bildmischung", "Richtung (Kamera)", "Beides"}, 1);
    Setting& strength_ = slider("strength", "Stärke", 0.5f, 0.05f, 1.f, "%.2f");
    Setting& samples_ = intSlider("samples", "Qualität (Proben)", 10, 4, 24);
    Setting& only_ = toggleSetting("only", "Nur bei Kamerabewegung", true);
    Setting& threshold_ = slider("threshold", "Schwelle (Pixel pro Frame)", 12.f, 2.f, 80.f, "%.0f");
    float motion_ = 0.f;
};
