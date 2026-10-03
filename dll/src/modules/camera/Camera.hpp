#pragma once

#include "gui/Gui.hpp"
#include "gui/Widgets.hpp"
#include "hook/GameInput.hpp"
#include "hook/Input.hpp"
#include "modules/Manager.hpp"
#include "modules/Module.hpp"
#include "modules/common/Bars.hpp"
#include "modules/common/Context.hpp"
#include "modules/common/Keys.hpp"
#include "modules/common/Needs.hpp"
#include "modules/post/PostFx.hpp"
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
        : Module("FOV Changer", "Set your field of view freely. Sprint and potion effects can be turned off.",
                 Category::Visual, {"camera"}) {
        sub("Camera");
        require(0, {fx::sig(fx::Id::Fov)});
        sprintBonus_.visible = [this] { return !noEffects_.b && need::have("MoveState"); };
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
    Setting& fov_ = slider("fov", "Field of view", 90.f, 30.f, 140.f, "%.0f");
    Setting& noEffects_ = toggleSetting("noEffects", "Sprint and potion effects off", true);
    Setting& sprintBonus_ = slider("sprintBonus", "Extra while sprinting", 6.f, 0.f, 30.f, "%.0f");
    Setting& smooth_ = slider("smooth", "Transition speed", 12.f, 1.f, 30.f, "%.0f");
    float current_ = 0.f;
};

class JavaDynamicFov : public Module {
public:
    JavaDynamicFov()
        : Module("Java Dynamic FOV", "Java-style FOV changes: sprinting, speed and drawing a bow change the view smoothly.", Category::Visual,
                 {"camera"}) {
        sub("Camera");
        require(need::player, {fx::sig(fx::Id::Fov), "LocalPlayer", "MoveState"});
        base_.visible = [] { return !game::has(game::Domain::Player); };
    }

    void onFrame() override {
        auto& p = game::state().player;
        float base = ctx::fovBase > 0.f ? ctx::fovBase : game::has(game::Domain::Player) ? p.fov : base_.f;
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
    Setting& base_ = slider("base", "Base field of view", 70.f, 30.f, 120.f, "%.0f");
    Setting& sprint_ = slider("sprint", "Extra while sprinting", 0.15f, 0.f, 0.5f, "%.2f");
    Setting& speed_ = slider("speed", "Potion effect strength", 1.f, 0.f, 2.f, "%.1fx");
    Setting& fly_ = slider("fly", "Extra while flying", 0.1f, 0.f, 0.5f, "%.2f");
    Setting& bow_ = slider("bow", "Zoom when drawing a bow", 0.15f, 0.f, 0.5f, "%.2f");
    float current_ = 1.f;
};

class Zoom : public Module {
public:
    Zoom()
        : Module("Zoom", "Zoom on a key with smooth animation, scroll wheel steps and adjusted sensitivity.", Category::Visual, {"camera"}) {
        sub("Camera");
        step_.visible = [this] { return scroll_.b; };
        base_.visible = [] { return !game::has(game::Domain::Player); };
        smooth_.visible = [this] { return !instant_.b; };
        sensAmount_.visible = [this] { return sens_.b; };
        hideHand_.visible = [] { return fx::available(fx::Id::HideHand); };
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
            snap_ = !always_.b;
            ev.cancel = true;
        }
    }

    void onFrame() override {
        if (zoom_.f != seenZoom_) {
            seenZoom_ = zoom_.f;
            level_ = zoom_.f;
        }
        float target = active_ ? level_ : 1.f;
        if ((snap_ || instant_.b) && active_) current_ = target;
        if (instant_.b && !active_) current_ = 1.f;
        snap_ = false;
        current_ = std::fabs(current_ - target) < 0.002f ? target : current_ + (target - current_) * std::min(1.f, ui::dt() * smooth_.f);
        ctx::zooming = current_ > 1.02f;
        ctx::zoomLevel = current_;
        ctx::hideModules = ctx::zooming && hideModules_.b;
        if (active_ && scroll_.b) gameinput::holdWheel();
        if (current_ <= 1.001f) return;
        if (hideHand_.b) fx::skip(fx::Id::HideHand);
        if (fx::available(fx::Id::Fov)) {
            float base = ctx::fovBase > 0.f ? ctx::fovBase : game::has(game::Domain::Player) ? game::state().player.fov : base_.f;
            float fov = 2.f * std::atan(std::tan(base * 0.0174533f * 0.5f) / current_) * 57.2958f;
            fx::set(fx::Id::Fov, fov);
            fx::set(fx::Id::FovEffects, 1.f);
        } else {
            post::params().zoom = std::max(post::params().zoom, current_);
        }
        if (!sens_.b) return;
        float k = 1.f / std::pow(current_, sensAmount_.f);
        if (fx::available(fx::Id::Sensitivity)) fx::scale(fx::Id::Sensitivity, k);
        else gameinput::scaleMouse(k);
    }

    void drawSettings() override {
        if (fx::available(fx::Id::Fov)) return;
        ImGui::Spacing();
        widgets::hint("Zooms the picture for now. With game data for your version it changes the real field of view, which looks sharper.");
    }

    void onRender(ImDrawList* dl) override {
        if (bars_.f > 0.f && current_ > 1.02f) bars::draw(dl, bars_.f * std::clamp((current_ - 1.f) / 1.5f, 0.f, 1.f));
        if (vignette_.f <= 0.f || current_ <= 1.02f) return;
        float k = std::clamp((current_ - 1.f) / 3.f, 0.f, 1.f) * vignette_.f;
        auto ds = ImGui::GetIO().DisplaySize;
        float e = std::min(ds.x, ds.y) * 0.35f;
        ImU32 solid = IM_COL32(0, 0, 0, int(200 * k)), clear = IM_COL32(0, 0, 0, 0);
        dl->AddRectFilledMultiColor({0, 0}, {ds.x, e}, solid, solid, clear, clear);
        dl->AddRectFilledMultiColor({0, ds.y - e}, {ds.x, ds.y}, clear, clear, solid, solid);
        dl->AddRectFilledMultiColor({0, 0}, {e, ds.y}, solid, clear, clear, solid);
        dl->AddRectFilledMultiColor({ds.x - e, 0}, {ds.x, ds.y}, clear, solid, solid, clear);
    }

    void onDisable() override {
        active_ = false;
        ctx::zooming = false;
        ctx::hideModules = false;
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

    Setting& key_ = keySetting("zoomKey", "Zoom key", 'C');
    Setting& mode_ = choice("mode", "Mode", {"Hold", "Toggle"});
    Setting& zoom_ = slider("zoom", "Zoom level", 4.f, 1.5f, 20.f, "%.1fx");
    Setting& base_ = slider("base", "Base field of view", 70.f, 30.f, 120.f, "%.0f");
    Setting& instant_ = toggleSetting("instant", "Instant (no animation)", false);
    Setting& smooth_ = slider("smooth", "Animation speed", 14.f, 2.f, 40.f, "%.0f");
    Setting& scroll_ = toggleSetting("scroll", "Change level with the mouse wheel", true);
    Setting& step_ = slider("step", "Step size", 1.2f, 1.05f, 1.6f, "%.2fx");
    Setting& remember_ = toggleSetting("remember", "Remember level", true);
    Setting& sens_ = toggleSetting("sens", "Adjust sensitivity while zooming", true);
    Setting& sensAmount_ = slider("sensAmount", "Sensitivity reduction", 0.85f, 0.f, 1.f, "%.2f");
    Setting& vignette_ = slider("vignette", "Dark edge while zooming", 0.f, 0.f, 1.f, "%.2f");
    Setting& bars_ = slider("bars", "Cinematic bars", 0.f, 0.f, 0.2f, "%.2f");
    Setting& hideHand_ = toggleSetting("hideHand", "Hide the hand while zooming", false);
    Setting& hideModules_ = toggleSetting("hideModules", "Hide the HUD modules while zooming", false);
    Setting& always_ = toggleSetting("always", "Always animate (also when scrolling)", true);
    bool snap_ = false;
    bool active_ = false;
    float seenZoom_ = -1.f;
    float level_ = 4.f;
    float current_ = 1.f;
};

class Freelook : public Module {
public:
    Freelook()
        : Module("Freelook", "Turn the camera freely around you while your body and walking direction stay. Banned on some servers.", Category::Visual, {"camera"}) {
        sub("Camera");
        require(need::player, need::sigs({"LocalPlayer", "FreeCamera"}));
    }

    void onKey(KeyEvent& ev) override {
        if (!key_.i || ev.vk != key_.i || ev.repeat) return;
        press(ev.down);
    }

    void onMouse(MouseEvent& ev) override {
        if (key_.i && mouseVk(ev.button) == key_.i) press(ev.down);
    }

    void onFrame() override {
        ctx::freelook = active_ && game::freeCamera(true);
        if (!active_) {
            game::freeCamera(false);
            return;
        }
        if (thirdPerson_.b) fx::setInt(fx::Id::Perspective, 1);
    }

    void onDisable() override {
        active_ = false;
        ctx::freelook = false;
        game::freeCamera(false);
    }

private:
    void press(bool down) { active_ = mode_.i == 0 ? down : (down ? !active_ : active_); }

    Setting& key_ = keySetting("freelookKey", "Freelook key", VK_LMENU);
    Setting& mode_ = choice("mode", "Mode", {"Hold", "Toggle"});
    Setting& thirdPerson_ = toggleSetting("thirdPerson", "Switch to third person", true);
    bool active_ = false;
};

class NoViewBobbing : public Module {
public:
    NoViewBobbing()
        : Module("No View Bobbing", "Turns off the bobbing while walking, separately for camera and hand.",
                 Category::Visual, {"camera"}) {
        sub("Camera");
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
    Setting& camera_ = toggleSetting("camera", "Camera bobbing off", true);
    Setting& hand_ = toggleSetting("hand", "Hand bobbing off", true);
    Setting& when_ = choice("when", "When", {"Always", "Only while sprinting", "Only underwater", "Only while walking"});
};

class MinimalViewBobbing : public Module {
public:
    MinimalViewBobbing()
        : Module("Minimal View Bobbing", "Weakens the bobbing while walking. You set the strength yourself.", Category::Visual, {"camera"}) {
        sub("Camera");
        require(0, {fx::sig(fx::Id::BobStrength)});
    }

    void onFrame() override {
        auto& p = game::state().player;
        float f = strength_.f * (p.sprinting ? sprint_.f : 1.f);
        fx::scale(fx::Id::BobStrength, f);
    }

private:
    Setting& strength_ = slider("strength", "Strength", 0.3f, 0.f, 1.f, "%.2f");
    Setting& sprint_ = slider("sprint", "Factor while sprinting", 1.f, 0.f, 2.f, "%.2fx");
};

class NoHurtCam : public Module {
public:
    NoHurtCam()
        : Module("No Hurt Cam", "The camera no longer shakes when you get hit. The strength is adjustable.", Category::Visual, {"camera"}) {
        sub("Camera");
        require(0, {fx::sig(fx::Id::HurtCam)});
    }

    void onFrame() override {
        if (strength_.f <= 0.001f) fx::skip(fx::Id::HurtCam);
        else fx::scale(fx::Id::HurtCam, strength_.f);
    }

private:
    Setting& strength_ = slider("strength", "Remaining strength", 0.f, 0.f, 1.f, "%.2f");
};

class SmoothSneak : public Module {
public:
    SmoothSneak()
        : Module("Smooth Sneak", "The camera no longer drops when you sneak.", Category::Visual, {"camera"}) {
        sub("Camera");
        require(0, {fx::sig(fx::Id::SneakCam)});
    }

    void onFrame() override { fx::scale(fx::Id::SneakCam, amount_.f); }

private:
    Setting& amount_ = slider("amount", "Remaining height change", 0.f, 0.f, 1.f, "%.2f");
};

class AutoPerspective : public Module {
public:
    AutoPerspective()
        : Module("Auto Perspective", "Switches the perspective automatically, for example when gliding or drawing a bow.",
                 Category::Visual, {"camera"}) {
        sub("Camera");
        require(need::player, {fx::sig(fx::Id::Perspective), "LocalPlayer", "MoveState"});
    }

    void onFrame() override {
        auto& p = game::state().player;
        int want = -1;
        if (p.emoting && emote_.i > 0) want = emote_.i - 1;
        else if (p.swimming && swim_.i > 0) want = swim_.i - 1;
        else if (p.gliding && glide_.i > 0) want = glide_.i - 1;
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
    Setting& swim_ = choice("swim", "While swimming", {"Do not change", "First person", "Third person back", "Third person front"});
    Setting& emote_ = choice("emote", "While emoting", {"Do not change", "First person", "Third person back", "Third person front"}, 2);
    Setting& glide_ = choice("glide", "While gliding", {"Do not change", "First person", "Third person back", "Third person front"}, 2);
    Setting& bow_ = choice("bow", "While drawing a bow", {"Do not change", "First person", "Third person back", "Third person front"}, 1);
    Setting& restore_ = toggleSetting("restore", "Switch back afterwards", true);
    int ctxView_ = -1;
};

class Fullbright : public Module {
public:
    Fullbright()
        : Module("Fullbright", "Maximum brightness everywhere, with a smooth fade and optionally only at night.", Category::Visual, {"camera"}) {
        sub("World");
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
    Setting& level_ = slider("level", "Level", 12.f, 1.f, 25.f, "%.0f");
    Setting& fade_ = toggleSetting("fade", "Smooth fade", true);
    Setting& onlyNight_ = toggleSetting("onlyNight", "Only at night", false);
    float current_ = 0.f;
};

class CinematicCamera : public Module {
public:
    CinematicCamera()
        : Module("Cinematic Camera", "Soft, gliding camera movement like in film shots, with black bars. Optionally only while zooming.", Category::Visual, {"camera"}) {
        sub("Camera");
    }

    void onFrame() override {
        bool on = !onlyZoom_.b || ctx::zooming;
        shown_ += ((on ? 1.f : 0.f) - shown_) * std::min(1.f, ui::dt() * 6.f);
        if (on) gameinput::smoothMouse(1.f - smoothing_.f);
    }

    void onRender(ImDrawList* dl) override { bars::draw(dl, bars_.f * shown_); }

    void onDisable() override { shown_ = 0.f; }

private:
    Setting& smoothing_ = slider("smoothing", "Smoothing", 0.7f, 0.f, 0.95f, "%.2f");
    Setting& onlyZoom_ = toggleSetting("onlyZoom", "Only while zooming", false);
    Setting& bars_ = slider("bars", "Cinematic bars", 0.08f, 0.f, 0.2f, "%.2f");
    float shown_ = 0.f;
};
