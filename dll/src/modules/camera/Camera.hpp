#pragma once

#include "gui/Gui.hpp"
#include "hook/Input.hpp"
#include "modules/Manager.hpp"
#include "modules/Module.hpp"
#include "modules/common/Context.hpp"
#include "modules/common/Keys.hpp"
#include "modules/common/Needs.hpp"
#include "render/Draw.hpp"
#include "render/Ui.hpp"
#include "sdk/Effects.hpp"
#include "sdk/Game.hpp"

#include <windows.h>

#include <algorithm>
#include <cmath>

class FovChanger : public Module {
public:
    FovChanger()
        : Module("FOV Changer", "Stellt dein Sichtfeld frei ein, auch über die normalen Grenzen hinaus. Sprint- und Trank-Effekte lassen sich abschalten.",
                 Category::Visual, {"camera"}) {
        sub("Kamera");
        require(0, {fx::sig(fx::Id::Fov)});
        sprintBonus_.visible = [this] { return !noEffects_.b; };
    }

    void onFrame() override {
        auto& p = game::state().player;
        float target = fov_.f + (p.sprinting && !noEffects_.b ? sprintBonus_.f : 0.f);
        float k = std::min(1.f, ui::dt() * smooth_.f);
        current_ = current_ <= 0.f ? target : current_ + (target - current_) * (smooth_.f >= 30.f ? 1.f : k);
        ctx::fovBase = current_;
        fx::set(fx::Id::Fov, current_);
        if (noEffects_.b) fx::set(fx::Id::FovEffects, 1.f);
    }

    void onDisable() override {
        ctx::fovBase = 0.f;
        current_ = 0.f;
    }

private:
    Setting& fov_ = slider("fov", "Sichtfeld", 90.f, 30.f, 140.f, "%.0f");
    Setting& noEffects_ = toggleSetting("noEffects", "Sprint- und Trank-Effekte aus", true);
    Setting& sprintBonus_ = slider("sprintBonus", "Zusatz beim Sprinten", 6.f, 0.f, 30.f, "%.0f");
    Setting& smooth_ = slider("smooth", "Übergangstempo", 12.f, 1.f, 30.f, "%.0f");
    float current_ = 0.f;
};

class JavaDynamicFov : public Module {
public:
    JavaDynamicFov()
        : Module("Java Dynamic FOV", "Sichtfeld-Änderungen wie in Java: Sprinten, Schnelligkeit und Bogenspannen verändern das Bild sanft.", Category::Visual,
                 {"camera"}) {
        sub("Kamera");
        require(need::player, {fx::sig(fx::Id::Fov), "LocalPlayer"});
    }

    void onFrame() override {
        auto& p = game::state().player;
        float base = ctx::fovBase > 0.f ? ctx::fovBase : base_.f;
        float mult = 1.f;
        if (p.sprinting) mult += sprint_.f;
        for (auto& e : p.effects) {
            if (e.id == "speed") mult += 0.1f * float(e.amplifier + 1) * speed_.f;
            if (e.id == "slowness") mult -= 0.15f * float(e.amplifier + 1) * speed_.f;
        }
        if (p.flying) mult += fly_.f;
        if (p.usingItem && p.held().name == "bow") {
            float t = std::clamp(p.useProgress, 0.f, 1.f);
            mult *= 1.f - bow_.f * t * t;
        }
        current_ += (mult - current_) * std::min(1.f, ui::dt() * 10.f);
        fx::set(fx::Id::Fov, base * current_);
        fx::set(fx::Id::FovEffects, 1.f);
    }

private:
    Setting& base_ = slider("base", "Basis-Sichtfeld", 70.f, 30.f, 120.f, "%.0f");
    Setting& sprint_ = slider("sprint", "Zusatz beim Sprinten", 0.15f, 0.f, 0.5f, "%.2f");
    Setting& speed_ = slider("speed", "Stärke der Trank-Effekte", 1.f, 0.f, 2.f, "%.1fx");
    Setting& fly_ = slider("fly", "Zusatz beim Fliegen", 0.1f, 0.f, 0.5f, "%.2f");
    Setting& bow_ = slider("bow", "Zoom beim Bogenspannen", 0.15f, 0.f, 0.5f, "%.2f");
    float current_ = 1.f;
};

class Zoom : public Module {
public:
    Zoom()
        : Module("Zoom", "Zoom per Taste mit weicher Animation, Scrollrad-Stufen und angepasster Empfindlichkeit.", Category::Visual, {"camera"}) {
        sub("Kamera");
        require(0, {fx::sig(fx::Id::Fov)});
        step_.visible = [this] { return scroll_.b; };
    }

    void onKey(KeyEvent& ev) override {
        if (!key_.i || ev.vk != key_.i || ev.repeat) return;
        press(ev.down);
    }

    void onMouse(MouseEvent& ev) override {
        if (mouseVk(ev.button) == key_.i && key_.i) press(ev.down);
        if (scroll_.b && active_ && ev.wheel != 0) {
            level_ *= std::pow(step_.f, ev.wheel > 0 ? 1.f : -1.f);
            level_ = std::clamp(level_, 1.2f, 40.f);
            ev.cancel = true;
        }
    }

    void onFrame() override {
        float target = active_ ? level_ : 1.f;
        current_ = std::fabs(current_ - target) < 0.002f ? target : current_ + (target - current_) * std::min(1.f, ui::dt() * smooth_.f);
        ctx::zooming = current_ > 1.02f;
        ctx::zoomLevel = current_;
        if (current_ <= 1.001f) return;
        float base = ctx::fovBase > 0.f ? ctx::fovBase : base_.f;
        float fov = 2.f * std::atan(std::tan(base * 0.0174533f * 0.5f) / current_) * 57.2958f;
        fx::set(fx::Id::Fov, fov);
        fx::set(fx::Id::FovEffects, 1.f);
        if (sens_.b) fx::scale(fx::Id::Sensitivity, 1.f / std::pow(current_, 0.85f));
    }

    void onDisable() override {
        active_ = false;
        ctx::zooming = false;
        current_ = 1.f;
    }

private:
    void press(bool down) {
        if (mode_.i == 0) {
            active_ = down;
        } else if (down) {
            active_ = !active_;
        }
        if (active_ && !remember_.b) level_ = zoom_.f;
    }

    Setting& key_ = keySetting("key", "Zoom-Taste", 'C');
    Setting& mode_ = choice("mode", "Modus", {"Halten", "Umschalten"});
    Setting& zoom_ = slider("zoom", "Zoomstufe", 4.f, 1.5f, 20.f, "%.1fx");
    Setting& base_ = slider("base", "Basis-Sichtfeld", 70.f, 30.f, 120.f, "%.0f");
    Setting& smooth_ = slider("smooth", "Animationstempo", 14.f, 2.f, 40.f, "%.0f");
    Setting& scroll_ = toggleSetting("scroll", "Stufe per Mausrad ändern", true);
    Setting& step_ = slider("step", "Schrittweite", 1.2f, 1.05f, 1.6f, "%.2fx");
    Setting& remember_ = toggleSetting("remember", "Stufe merken", true);
    Setting& sens_ = toggleSetting("sens", "Empfindlichkeit beim Zoomen anpassen", true);
    bool active_ = false;
    float level_ = 4.f;
    float current_ = 1.f;
};

class Freelook : public Module {
public:
    Freelook()
        : Module("Freelook", "Kamera frei um dich drehen, während Körper und Laufrichtung bleiben. Auf manchen Servern verboten.", Category::Visual, {"camera"}) {
        sub("Kamera");
        require(need::player, {fx::sig(fx::Id::LookCamera), fx::sig(fx::Id::LookTurn), "LocalPlayer"});
    }

    void onKey(KeyEvent& ev) override {
        if (!key_.i || ev.vk != key_.i || ev.repeat) return;
        press(ev.down);
    }

    void onMouse(MouseEvent& ev) override {
        if (key_.i && mouseVk(ev.button) == key_.i) press(ev.down);
    }

    void onFrame() override {
        if (!active_) {
            ctx::freelook = false;
            return;
        }
        if (gui::open()) return;
        auto d = modules::mouseDelta();
        float k = 0.12f * sens_.f;
        yaw_ += float(d.x) * k;
        pitch_ += float(d.y) * k * (invert_.b ? -1.f : 1.f);
        pitch_ = std::clamp(pitch_, -90.f, 90.f);
        ctx::freelook = true;
        fx::skip(fx::Id::LookTurn);
        fx::out(fx::Id::LookCamera, {pitch_, yaw_});
    }

    void onDisable() override {
        active_ = false;
        ctx::freelook = false;
    }

private:
    void press(bool down) {
        bool next = mode_.i == 0 ? down : (down ? !active_ : active_);
        if (next && !active_) {
            auto& p = game::state().player;
            yaw_ = p.yaw;
            pitch_ = p.pitch;
        }
        active_ = next;
    }

    Setting& key_ = keySetting("key", "Freelook-Taste", VK_LMENU);
    Setting& mode_ = choice("mode", "Modus", {"Halten", "Umschalten"});
    Setting& sens_ = slider("sens", "Empfindlichkeit", 1.f, 0.2f, 3.f, "%.2fx");
    Setting& invert_ = toggleSetting("invert", "Y-Achse umkehren", false);
    float yaw_ = 0.f;
    float pitch_ = 0.f;
    bool active_ = false;
};

class NoViewBobbing : public Module {
public:
    NoViewBobbing()
        : Module("No View Bobbing", "Schaltet das Wackeln beim Laufen komplett ab, getrennt für Kamera und Hand, wahlweise nur beim Sprinten oder unter Wasser.",
                 Category::Visual, {"camera"}) {
        sub("Kamera");
        require(0, {fx::sig(fx::Id::ViewBob)});
    }

    void onFrame() override {
        auto& p = game::state().player;
        bool on = when_.i == 0 || (when_.i == 1 && p.sprinting) || (when_.i == 2 && p.inWater) || (when_.i == 3 && !p.sprinting);
        if (!on) return;
        if (camera_.b) fx::skip(fx::Id::ViewBob);
        if (hand_.b) fx::skip(fx::Id::HandBob);
    }

private:
    Setting& camera_ = toggleSetting("camera", "Kamera-Wackeln aus", true);
    Setting& hand_ = toggleSetting("hand", "Hand-Wackeln aus", true);
    Setting& when_ = choice("when", "Wann", {"Immer", "Nur beim Sprinten", "Nur unter Wasser", "Nur beim Gehen"});
};

class MinimalViewBobbing : public Module {
public:
    MinimalViewBobbing()
        : Module("Minimal View Bobbing", "Schwächt das Wackeln beim Laufen ab. Die Stärke stellst du selbst ein.", Category::Visual, {"camera"}) {
        sub("Kamera");
        require(0, {fx::sig(fx::Id::BobStrength)});
    }

    void onFrame() override {
        auto& p = game::state().player;
        float f = strength_.f * (p.sprinting ? sprint_.f : 1.f);
        fx::scale(fx::Id::BobStrength, f);
    }

private:
    Setting& strength_ = slider("strength", "Stärke", 0.3f, 0.f, 1.f, "%.2f");
    Setting& sprint_ = slider("sprint", "Faktor beim Sprinten", 1.f, 0.f, 2.f, "%.2fx");
};

class NoHurtCam : public Module {
public:
    NoHurtCam()
        : Module("No Hurt Cam", "Die Kamera wackelt nicht mehr, wenn du getroffen wirst. Die Stärke ist regelbar.", Category::Visual, {"camera"}) {
        sub("Kamera");
        require(0, {fx::sig(fx::Id::HurtCam)});
    }

    void onFrame() override {
        if (strength_.f <= 0.001f) fx::skip(fx::Id::HurtCam);
        else fx::scale(fx::Id::HurtCam, strength_.f);
    }

private:
    Setting& strength_ = slider("strength", "Restliche Stärke", 0.f, 0.f, 1.f, "%.2f");
};

class SmoothSneak : public Module {
public:
    SmoothSneak()
        : Module("Smooth Sneak", "Die Kamera springt beim Schleichen nicht mehr nach unten.", Category::Visual, {"camera"}) {
        sub("Kamera");
        require(0, {fx::sig(fx::Id::SneakCam)});
    }

    void onFrame() override { fx::scale(fx::Id::SneakCam, amount_.f); }

private:
    Setting& amount_ = slider("amount", "Restliche Höhenänderung", 0.f, 0.f, 1.f, "%.2f");
};

class AutoPerspective : public Module {
public:
    AutoPerspective()
        : Module("Auto Perspective", "Wechselt die Perspektive automatisch, etwa beim Gleiten mit Elytra oder beim Bogenspannen, und danach zurück.",
                 Category::Visual, {"camera"}) {
        sub("Kamera");
        require(need::player, {fx::sig(fx::Id::Perspective), "LocalPlayer"});
    }

    void onFrame() override {
        auto& p = game::state().player;
        int want = -1;
        if (p.gliding && glide_.i > 0) want = glide_.i - 1;
        else if (p.usingItem && p.held().name == "bow" && bow_.i > 0) want = bow_.i - 1;
        if (want >= 0) {
            fx::setInt(fx::Id::Perspective, want);
            ctxView_ = want;
        } else if (ctxView_ >= 0) {
            if (restore_.b) fx::setInt(fx::Id::Perspective, int(p.view));
            ctxView_ = -1;
        }
    }

private:
    Setting& glide_ = choice("glide", "Beim Gleiten", {"Nichts ändern", "Erste Person", "Dritte Person hinten", "Dritte Person vorne"}, 2);
    Setting& bow_ = choice("bow", "Beim Bogenspannen", {"Nichts ändern", "Erste Person", "Dritte Person hinten", "Dritte Person vorne"}, 1);
    Setting& restore_ = toggleSetting("restore", "Danach zurückwechseln", true);
    int ctxView_ = -1;
};

class Fullbright : public Module {
public:
    Fullbright()
        : Module("Fullbright", "Maximale Helligkeit überall, mit weichem Übergang und optional nur nachts oder in Höhlen.", Category::Visual, {"camera"}) {
        sub("Welt");
        require(0, {fx::sig(fx::Id::Gamma)});
    }

    void onFrame() override {
        auto& w = game::state().world;
        bool dark = !onlyNight_.b || !game::has(game::Domain::World) || w.time >= 12500 || w.time < 500;
        float target = dark ? level_.f : 0.f;
        current_ += (target - current_) * std::min(1.f, ui::dt() * (fade_.b ? 4.f : 60.f));
        if (current_ > 0.05f) fx::set(fx::Id::Gamma, std::max(current_, 1.f));
    }

    void onDisable() override { current_ = 0.f; }

private:
    Setting& level_ = slider("level", "Stufe", 12.f, 1.f, 25.f, "%.0f");
    Setting& fade_ = toggleSetting("fade", "Sanfter Übergang", true);
    Setting& onlyNight_ = toggleSetting("onlyNight", "Nur nachts", false);
    float current_ = 0.f;
};
