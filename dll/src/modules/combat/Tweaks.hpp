#pragma once

#include "core/Config.hpp"
#include "hook/Input.hpp"
#include "modules/Module.hpp"
#include "modules/common/Context.hpp"
#include "modules/common/Needs.hpp"
#include "sdk/Effects.hpp"
#include "sdk/Game.hpp"
#include "sdk/Inject.hpp"

#include <windows.h>

#include <cmath>

class HurtColor : public Module {
public:
    HurtColor()
        : Module("Hurt Color", "Changes the red tint when you or an opponent take damage.", Category::Pvp, {"cosmetic"}) {
        sub("Hit visuals");
        require(0, {fx::sig(fx::Id::HurtColor)});
        speed_.visible = [this] { return rainbow_.b; };
        color_.visible = [this] { return !rainbow_.b; };
    }

    void onFrame() override {
        ImVec4 c = color_.color;
        if (rainbow_.b) {
            float r, g, b;
            ImGui::ColorConvertHSVtoRGB(std::fmod(float(ui::time()) * speed_.f * 0.2f, 1.f), 0.7f, 1.f, r, g, b);
            c = {r, g, b, 1.f};
        }
        fx::out(fx::Id::HurtColor, {c.x, c.y, c.z, c.w * intensity_.f});
    }

private:
    Setting& color_ = colorSetting("color", "Color", {1.f, 0.35f, 0.6f, 1.f});
    Setting& intensity_ = slider("intensity", "Strength", 0.6f, 0.05f, 1.f, "%.2f");
    Setting& rainbow_ = toggleSetting("rainbow", "Rainbow", false);
    Setting& speed_ = slider("speed", "Speed", 1.f, 0.1f, 5.f, "%.1f");
};

class GlintColor : public Module {
public:
    GlintColor()
        : Module("Glint Color", "Changes the color of the enchantment glint on items and armor.", Category::Pvp, {"cosmetic"}) {
        sub("Hit visuals");
        require(0, {fx::sig(fx::Id::GlintColor)});
        speed_.visible = [this] { return rainbow_.b; };
        color_.visible = [this] { return !rainbow_.b; };
    }

    void onFrame() override {
        ImVec4 c = color_.color;
        if (rainbow_.b) {
            float r, g, b;
            ImGui::ColorConvertHSVtoRGB(std::fmod(float(ui::time()) * speed_.f * 0.2f, 1.f), 0.6f, 1.f, r, g, b);
            c = {r, g, b, 1.f};
        }
        fx::out(fx::Id::GlintColor, {c.x, c.y, c.z, c.w});
    }

private:
    Setting& color_ = colorSetting("color", "Color", {1.f, 0.49f, 0.71f, 1.f});
    Setting& rainbow_ = toggleSetting("rainbow", "Rainbow", false);
    Setting& speed_ = slider("speed", "Speed", 1.f, 0.1f, 5.f, "%.1f");
};

class Hitbox : public Module {
public:
    Hitbox()
        : Module("Hitbox", "Shows hitboxes with the game's own drawing path, so never through walls. Box style, thickness, opacity, eye and look lines, range, your own box and Java size.",
                 Category::Pvp, {"info-others"}) {
        sub("Hit visuals");
        require(0, {fx::sig(fx::Id::Hitbox)});
        color_.visible = [this] { return !rainbow_.b; };
        style_.visible = [] { return fx::available(fx::Id::Hitbox2D); };
        width_.visible = [] { return fx::available(fx::Id::HitboxWidth); };
        range_.visible = [] { return fx::available(fx::Id::HitboxRange); };
        self_.visible = [] { return fx::available(fx::Id::HitboxSelf); };
        eye_.visible = [] { return fx::available(fx::Id::HitboxEye); };
        eyeColor_.visible = [this] { return eye_.b && fx::available(fx::Id::HitboxEyeColor); };
        look_.visible = [] { return fx::available(fx::Id::HitboxLook); };
        lookLength_.visible = [this] { return look_.b && fx::available(fx::Id::HitboxLookLength); };
        lookColor_.visible = [this] { return look_.b && fx::available(fx::Id::HitboxLookColor); };
        java_.visible = [] { return fx::available(fx::Id::HitboxJava); };
        javaKey_.visible = [] { return fx::available(fx::Id::HitboxJava); };
    }

    void onKey(KeyEvent& ev) override {
        if (!ev.down || ev.repeat || !javaKey_.i || ev.vk != javaKey_.i) return;
        java_.b = !java_.b;
        config::markDirty();
    }

    void onFrame() override {
        fx::force(fx::Id::Hitbox, true);
        ImVec4 c = color_.color;
        if (rainbow_.b) {
            float r, g, b;
            ImGui::ColorConvertHSVtoRGB(std::fmod(float(ui::time()) * 0.3f, 1.f), 0.7f, 1.f, r, g, b);
            c = {r, g, b, 1.f};
        }
        fx::out(fx::Id::HitboxColor, {c.x, c.y, c.z, c.w * opacity_.f});
        if (style_.i == 1) fx::force(fx::Id::Hitbox2D, true);
        fx::set(fx::Id::HitboxWidth, width_.f);
        fx::set(fx::Id::HitboxRange, range_.f);
        if (self_.b) fx::force(fx::Id::HitboxSelf, true);
        if (java_.b) fx::force(fx::Id::HitboxJava, true);
        if (eye_.b) {
            fx::force(fx::Id::HitboxEye, true);
            fx::out(fx::Id::HitboxEyeColor, {eyeColor_.color.x, eyeColor_.color.y, eyeColor_.color.z, eyeColor_.color.w * opacity_.f});
        }
        if (look_.b) {
            fx::force(fx::Id::HitboxLook, true);
            fx::set(fx::Id::HitboxLookLength, lookLength_.f);
            fx::out(fx::Id::HitboxLookColor, {lookColor_.color.x, lookColor_.color.y, lookColor_.color.z, lookColor_.color.w * opacity_.f});
        }
    }

private:
    Setting& style_ = choice("style", "Box style", {"3D box", "Flat (2D)"});
    Setting& opacity_ = slider("opacity", "Opacity", 1.f, 0.1f, 1.f, "%.2f");
    Setting& width_ = slider("width", "Line thickness", 2.f, 0.5f, 8.f, "%.1f");
    Setting& range_ = slider("range", "Range (blocks)", 30.f, 5.f, 30.f, "%.0f");
    Setting& self_ = toggleSetting("self", "Show your own hitbox", false);
    Setting& rainbow_ = toggleSetting("rainbow", "Rainbow", false);
    Setting& eye_ = toggleSetting("eye", "Eye line", false);
    Setting& look_ = toggleSetting("look", "Look direction line", false);
    Setting& lookLength_ = slider("lookLength", "Look line length", 3.f, 0.5f, 10.f, "%.1f");
    Setting& java_ = toggleSetting("java", "Java-style size (+0.1)", false);
    Setting& javaKey_ = keySetting("javaKey", "Java size key", 0);
    Setting& color_ = colorSetting("color", "Color", {1.f, 0.49f, 0.71f, 1.f});
    Setting& eyeColor_ = colorSetting("eyeColor", "Eye line color", {1.f, 0.3f, 0.3f, 1.f});
    Setting& lookColor_ = colorSetting("lookColor", "Look line color", {0.3f, 0.5f, 1.f, 1.f});
};

class LowFire : public Module {
public:
    LowFire()
        : Module("Low Fire", "Lowers the fire overlay at the bottom of the screen or hides it completely.", Category::Pvp, {"cosmetic"}) {
        sub("Hit visuals");
        require(0, {fx::sig(fx::Id::FireHeight)});
    }

    void onFrame() override { fx::scale(fx::Id::FireHeight, 1.f - amount_.f); }

private:
    Setting& amount_ = slider("amount", "Lower", 0.6f, 0.f, 1.f, "%.2f");
};

class ParticleMultiplier : public Module {
public:
    ParticleMultiplier()
        : Module("Particle Multiplier", "More or fewer particles, and critical hit particles on every hit. Fewer helps FPS, more looks nicer.", Category::Pvp,
                 {"cosmetic"}) {
        sub("Hit feedback");
        requireAny({fx::sig(fx::Id::ParticleScale), fx::sig(fx::Id::CritParticle)});
        amount_.visible = [] { return fx::available(fx::Id::ParticleScale); };
        crit_.visible = [] { return fx::available(fx::Id::CritParticle); };
    }

    void onFrame() override {
        fx::set(fx::Id::ParticleScale, amount_.f);
        if (crit_.b) fx::force(fx::Id::CritParticle, true);
    }

private:
    Setting& amount_ = slider("amount", "Amount", 0.5f, 0.f, 4.f, "%.2fx");
    Setting& crit_ = toggleSetting("normalCrit", "Critical hit particles on every hit", false);
};

class SensMultiplier : public Module {
public:
    SensMultiplier()
        : Module("Sens Multiplier", "Own sensitivity while zooming, drawing a bow, blocking, sneaking and sprinting.",
                 Category::Pvp, {"input"}) {
        sub("Input");
        require(0, {fx::sig(fx::Id::Sensitivity)});
    }

    void onFrame() override {
        auto& p = game::state().player;
        float m = base_.f;
        if (ctx::zooming) m *= zoom_.f;
        if (p.usingItem && p.held().name == "bow") m *= bow_.f;
        if (p.blocking) m *= block_.f;
        if (p.sneaking) m *= sneak_.f;
        if (p.sprinting) m *= sprint_.f;
        fx::scale(fx::Id::Sensitivity, m);
    }

private:
    Setting& base_ = slider("base", "Base value", 1.f, 0.2f, 3.f, "%.2fx");
    Setting& zoom_ = slider("zoom", "While zooming", 0.6f, 0.1f, 2.f, "%.2fx");
    Setting& bow_ = slider("bow", "While drawing a bow", 1.f, 0.1f, 2.f, "%.2fx");
    Setting& block_ = slider("block", "While blocking", 1.f, 0.1f, 2.f, "%.2fx");
    Setting& sneak_ = slider("sneak", "While sneaking", 1.f, 0.1f, 2.f, "%.2fx");
    Setting& sprint_ = slider("sprint", "While sprinting", 1.f, 0.1f, 2.f, "%.2fx");
};

class BowSensitivity : public Module {
public:
    BowSensitivity()
        : Module("Bow Sensitivity", "Own sensitivity while you draw the bow, for more precise aiming.", Category::Pvp, {"input"}) {
        sub("Input");
        require(need::player, {fx::sig(fx::Id::Sensitivity), "LocalPlayer"});
    }

    void onFrame() override {
        auto& p = game::state().player;
        if (p.usingItem && (p.held().name == "bow" || !onlyBow_.b)) fx::scale(fx::Id::Sensitivity, factor_.f);
    }

private:
    Setting& factor_ = slider("factor", "Sensitivity", 0.7f, 0.1f, 2.f, "%.2fx");
    Setting& onlyBow_ = toggleSetting("onlyBow", "Only with a bow", true);
};

class SnapLook : public Module {
public:
    SnapLook()
        : Module("Snap Look", "A short look behind or to the side on a key, without changing your aim.",
                 Category::Pvp, {"camera"}) {
        sub("Input");
        require(need::player, {fx::sig(fx::Id::LookCamera), "LocalPlayer"});
    }

    void onKey(KeyEvent& ev) override {
        if (ev.vk != key_.i || !key_.i || ev.repeat) return;
        if (mode_.i == 0) active_ = ev.down;
        else if (ev.down) active_ = !active_;
    }

    void onFrame() override {
        ctx::freelook = false;
        if (!active_) return;
        static const float yaws[] = {180.f, 90.f, -90.f};
        auto& p = game::state().player;
        fx::out(fx::Id::LookCamera, {keepPitch_.b ? p.pitch : 0.f, p.yaw + yaws[dir_.i]});
    }

    void onDisable() override { active_ = false; }

private:
    Setting& key_ = keySetting("snapKey", "Look key", 'V');
    Setting& mode_ = choice("mode", "Mode", {"Hold", "Toggle"});
    Setting& dir_ = choice("dir", "Direction", {"Behind", "Left", "Right"});
    Setting& keepPitch_ = toggleSetting("keepPitch", "Keep pitch", true);
    bool active_ = false;
};

class NullMovement : public Module {
public:
    NullMovement()
        : Module("Null Movement",
                 "Opposite direction keys no longer cancel each other. Banned on many servers.",
                 Category::Pvp, {"input"}) {
        sub("Input");
        markRisky();
    }

    void onKey(KeyEvent& ev) override {
        if (inject::ours()) return;
        for (int axis = 0; axis < 2; axis++) {
            if (axis == 0 && !horizontal_.b) continue;
            if (axis == 1 && !vertical_.b) continue;
            int a = axis == 0 ? left_.i : forward_.i, b = axis == 0 ? right_.i : back_.i;
            if (ev.vk == a) handle(axis, 0, ev);
            else if (ev.vk == b) handle(axis, 1, ev);
        }
    }

    void onDisable() override { releaseAll(); }

private:
    int key(int axis, int side) const {
        if (axis == 0) return side == 0 ? left_.i : right_.i;
        return side == 0 ? forward_.i : back_.i;
    }

    void handle(int axis, int side, KeyEvent& ev) {
        int other = 1 - side;
        if (ev.down && ev.repeat) {
            if (cut_[axis][side]) ev.cancel = true;
            return;
        }
        if (ev.down) {
            held_[axis][side] = true;
            if (!held_[axis][other]) return;
            if (!cut_[axis][other]) {
                inject::key(key(axis, other), false);
                cut_[axis][other] = true;
            }
            if (mode_.i == 1) {
                cut_[axis][side] = true;
                ev.cancel = true;
            }
            return;
        }
        held_[axis][side] = false;
        if (cut_[axis][side]) {
            cut_[axis][side] = false;
            ev.cancel = true;
        }
        if (held_[axis][other] && cut_[axis][other]) {
            inject::key(key(axis, other), true);
            cut_[axis][other] = false;
        }
    }

    void releaseAll() {
        for (int axis = 0; axis < 2; axis++)
            for (int side = 0; side < 2; side++) {
                if (cut_[axis][side] && held_[axis][side]) inject::key(key(axis, side), true);
                cut_[axis][side] = false;
                held_[axis][side] = false;
            }
    }

    Setting& mode_ = choice("mode", "Behavior", {"Last pressed key wins", "Both cancel out"});
    Setting& horizontal_ = toggleSetting("horizontal", "Left / Right", true);
    Setting& vertical_ = toggleSetting("vertical", "Forward / Back", false);
    Setting& left_ = keySetting("left", "Left", 'A');
    Setting& right_ = keySetting("right", "Right", 'D');
    Setting& forward_ = keySetting("forward", "Forward", 'W');
    Setting& back_ = keySetting("back", "Back", 'S');
    bool held_[2][2]{};
    bool cut_[2][2]{};
};

class ItemUseDelayFix : public Module {
public:
    ItemUseDelayFix()
        : Module("Item Use Delay Fix", "Shortens the wait between item uses. Banned on many servers.",
                 Category::Pvp, {"timing"}) {
        sub("Input");
        markRisky();
        require(0, {fx::sig(fx::Id::UseDelay)});
    }

    void onFrame() override { fx::scale(fx::Id::UseDelay, factor_.f); }

private:
    Setting& factor_ = slider("factor", "Remaining wait", 0.5f, 0.f, 1.f, "%.2f");
};

class FasterInventory : public Module {
public:
    FasterInventory()
        : Module("Faster Inventory", "Faster moving of items in the inventory. Banned on many servers.",
                 Category::Pvp, {"timing"}) {
        sub("Input");
        markRisky();
        require(0, {fx::sig(fx::Id::InventoryDelay)});
    }

    void onFrame() override { fx::scale(fx::Id::InventoryDelay, factor_.f); }

private:
    Setting& factor_ = slider("factor", "Remaining wait", 0.5f, 0.f, 1.f, "%.2f");
};

class InstaHurtAnimation : public Module {
public:
    InstaHurtAnimation()
        : Module("Insta Hurt Animation", "Plays the hurt animation of the player you hit right away instead of after the server reply. Banned on many servers.",
                 Category::Pvp, {"timing"}) {
        sub("Hit feedback");
        markRisky();
        require(0, {fx::sig(fx::Id::HurtAnim)});
        wants(need::target);
        excludeTeam_.visible = [] { return game::ready(need::target); };
        fullArmor_.visible = [] { return game::ready(need::target); };
    }

    void onFrame() override {
        auto& st = game::state();
        auto& t = st.target;
        bool filtered = game::ready(need::target) && (excludeTeam_.b || fullArmor_.b);
        if (filtered) {
            if (t.kind != game::Target::Kind::Entity || !t.isPlayer) return;
            if (excludeTeam_.b && st.player.team && t.team == st.player.team) return;
            if (fullArmor_.b && t.armor < 4) return;
        }
        fx::force(fx::Id::HurtAnim, true);
    }

private:
    Setting& excludeTeam_ = toggleSetting("excludeTeam", "Exclude team", true);
    Setting& fullArmor_ = toggleSetting("fullArmor", "Only against full armor", false);
};
