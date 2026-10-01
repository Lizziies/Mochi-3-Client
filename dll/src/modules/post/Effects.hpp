#pragma once

#include "PostFx.hpp"
#include "gui/Gui.hpp"
#include "modules/Manager.hpp"
#include "modules/Module.hpp"
#include "render/Ui.hpp"

#include <windows.h>

#include <algorithm>
#include <cmath>

class SaturationHue : public Module {
public:
    SaturationHue()
        : Module("Saturation / Hue", "Changes saturation and hue of the game image, with presets and an optional rainbow cycle.",
                 Category::Visual, {"cosmetic"}) {
        sub("Post effects");
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
    Setting& preset_ = choice("preset", "Preset", {"Own values", "Black and white", "Vivid", "Pastel", "Warm", "Retro"});
    Setting& saturation_ = slider("saturation", "Saturation", 1.2f, 0.f, 2.5f, "%.2f");
    Setting& hue_ = slider("hue", "Hue", 0.f, -180.f, 180.f, "%.0f°");
    Setting& cycle_ = toggleSetting("cycle", "Cycle the hue", false);
    Setting& speed_ = slider("speed", "Speed", 0.5f, 0.05f, 3.f, "%.2f");
    float phase_ = 0.f;
};

class BrightnessContrast : public Module {
public:
    BrightnessContrast()
        : Module("Brightness / Contrast", "Brightness, contrast, gamma and vignette for the game image.",
                 Category::Visual, {"cosmetic"}) {
        sub("Post effects");
    }

    void onFrame() override {
        auto& p = post::params();
        p.brightness += brightness_.f;
        p.contrast *= contrast_.f;
        p.gamma *= gamma_.f;
        p.vignette = std::max(p.vignette, vignette_.f);
    }

private:
    Setting& brightness_ = slider("brightness", "Brightness", 0.05f, -0.5f, 0.5f, "%.2f");
    Setting& contrast_ = slider("contrast", "Contrast", 1.1f, 0.5f, 2.f, "%.2f");
    Setting& gamma_ = slider("gamma", "Gamma", 1.f, 0.5f, 2.5f, "%.2f");
    Setting& vignette_ = slider("vignette", "Vignette", 0.f, 0.f, 1.f, "%.2f");
};

class ScreenTint : public Module {
public:
    ScreenTint()
        : Module("Screen Tint", "Puts a color filter over the game image, for example a soft pink.",
                 Category::Visual, {"cosmetic"}) {
        sub("Post effects");
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
    Setting& color_ = colorSetting("color", "Color", {1.f, 0.55f, 0.75f, 1.f});
    Setting& strength_ = slider("strength", "Strength", 0.14f, 0.f, 1.f, "%.2f");
    Setting& mode_ = choice("mode", "Blend mode", {"Mix", "Multiply", "Add", "Overlay"});
    Setting& pulse_ = toggleSetting("pulse", "Pulse", false);
    Setting& pulseSpeed_ = slider("pulseSpeed", "Pulse speed (Hz)", 0.4f, 0.05f, 3.f, "%.2f");
    Setting& pulseDepth_ = slider("pulseDepth", "Pulse depth", 0.5f, 0.f, 1.f, "%.2f");
    float phase_ = 0.f;
};

class Sharpen : public Module {
public:
    Sharpen()
        : Module("Sharpen", "Sharpens the game image, useful at low resolution or scaling.", Category::Visual,
                 {"cosmetic"}) {
        sub("Post effects");
    }

    void onFrame() override { post::params().sharpen += amount_.f; }

private:
    Setting& amount_ = slider("amount", "Strength", 0.6f, 0.f, 3.f, "%.2f");
};

class DepthOfField : public Module {
public:
    DepthOfField()
        : Module("Depth of Field", "Soft focus: the image gets blurry towards the edges, like a camera with a tilt-shift effect.",
                 Category::Visual, {"cosmetic"}) {
        sub("Post effects");
    }

    void onFrame() override { post::params().dof = std::max(post::params().dof, amount_.f); }

private:
    Setting& amount_ = slider("amount", "Strength", 0.5f, 0.f, 1.f, "%.2f");
};

class Blur : public Module {
public:
    Blur()
        : Module("Blur", "Blurs the game image, always or only while a menu, the inventory or the chat is open.",
                 Category::Visual, {"cosmetic"}) {
        sub("Post effects");
    }

    void onFrame() override {
        bool menu = gui::open() || game::state().screen != game::Screen::None;
        if (when_.i == 1 && !menu) return;
        float px = amount_.f * 24.f * ui::scale();
        post::params().blur = std::max(post::params().blur, px);
    }

private:
    Setting& amount_ = slider("amount", "Strength", 0.5f, 0.05f, 1.f, "%.2f");
    Setting& when_ = choice("when", "When", {"Always", "Only in menus"}, 1);
};

class ColorFilter : public Module {
public:
    ColorFilter()
        : Module("Color Filter", "Color filter for color blindness: corrects or simulates red, green and blue weakness.",
                 Category::Visual, {"cosmetic"}) {
        sub("Post effects");
    }

    void onFrame() override {
        int base = type_.i + 1;
        post::params().colorMode = base + (simulate_.b ? 3 : 0);
    }

private:
    Setting& type_ = choice("type", "Type", {"Red weakness (protanopia)", "Green weakness (deuteranopia)", "Blue weakness (tritanopia)"});
    Setting& simulate_ = toggleSetting("simulate", "Only simulate instead of correcting", false);
};

class NightShift : public Module {
public:
    NightShift()
        : Module("Night Shift", "A warmer blue-light filter for relaxed eyes, optionally automatic in the evening.", Category::Visual,
                 {"cosmetic"}) {
        sub("Post effects");
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

    Setting& temperature_ = slider("temperature", "Color temperature (K)", 3400.f, 1500.f, 6500.f, "%.0f");
    Setting& strength_ = slider("strength", "Strength", 0.8f, 0.f, 1.f, "%.2f");
    Setting& auto_ = toggleSetting("auto", "Only at certain hours", false);
    Setting& from_ = slider("from", "From (hour)", 20.f, 0.f, 24.f, "%.1f");
    Setting& to_ = slider("to", "Until (hour)", 7.f, 0.f, 24.f, "%.1f");
};

class MotionBlur : public Module {
public:
    MotionBlur()
        : Module("Motion Blur", "Motion blur from camera movement and frame blending. Strength, quality and threshold adjustable.",
                 Category::Visual, {"cosmetic"}) {
        sub("Post effects");
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
    Setting& mode_ = choice("mode", "Type", {"Frame blending", "Direction (camera)", "Both"}, 1);
    Setting& strength_ = slider("strength", "Strength", 0.5f, 0.05f, 1.f, "%.2f");
    Setting& samples_ = intSlider("samples", "Quality (samples)", 10, 4, 24);
    Setting& only_ = toggleSetting("only", "Only while the camera moves", true);
    Setting& threshold_ = slider("threshold", "Threshold (pixels per frame)", 12.f, 2.f, 80.f, "%.0f");
    float motion_ = 0.f;
};
