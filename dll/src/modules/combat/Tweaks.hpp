#pragma once

#include "core/Config.hpp"
#include "gui/Widgets.hpp"
#include "hook/GameInput.hpp"
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
    Setting& color_ = colorSetting("color", "Color", {0.23f, 0.65f, 0.93f, 1.f});
    Setting& rainbow_ = toggleSetting("rainbow", "Rainbow", false);
    Setting& speed_ = slider("speed", "Speed", 1.f, 0.1f, 5.f, "%.1f");
};

class Hitbox : public Module {
public:
    Hitbox()
        : Module("Hitbox", "Shows the hitbox of what you aim at, and your own in third person. Only what you can see, never through walls. Box or flat style, thickness, opacity, eye and look lines, Java size.",
                 Category::Pvp, {"info-others"}) {
        sub("Hit visuals");
        require(need::target | need::camera, need::sigs({"Target", "TargetBox"}));
        color_.visible = [this] { return !rainbow_.b; };
        eyeColor_.visible = [this] { return eye_.b; };
        lookLength_.visible = [this] { return look_.b; };
        lookColor_.visible = [this] { return look_.b; };
    }

    void onKey(KeyEvent& ev) override {
        if (!ev.down || ev.repeat || !javaKey_.i || ev.vk != javaKey_.i) return;
        java_.b = !java_.b;
        config::markDirty();
    }

    void onRender(ImDrawList* dl) override {
        auto& st = game::state();
        if (!st.inWorld) return;
        ImVec4 c = color_.color;
        if (rainbow_.b) {
            float r, g, b;
            ImGui::ColorConvertHSVtoRGB(std::fmod(float(ui::time()) * 0.3f, 1.f), 0.7f, 1.f, r, g, b);
            c = {r, g, b, 1.f};
        }
        c.w *= opacity_.f;
        auto& t = st.target;
        // the pick ray stops at the first thing in the way, so the targeted entity is always in plain sight
        if (t.kind == game::Target::Kind::Entity && t.hasBox) drawBox(dl, t.boxMin, t.boxMax, c, t.lookYaw, t.lookPitch, true);
        auto& me = st.player;
        if (self_.b && me.hasBox && me.view != game::View::First) drawBox(dl, me.boxMin, me.boxMax, c, me.yaw, me.pitch, false);
    }

private:
    void drawBox(ImDrawList* dl, game::Vec3 lo, game::Vec3 hi, ImVec4 c, float yaw, float pitch, bool other) {
        float grow = java_.b ? 0.1f : 0.f;
        lo = {lo.x - grow, lo.y - grow, lo.z - grow};
        hi = {hi.x + grow, hi.y + grow, hi.z + grow};
        float w = width_.f * ui::scale();
        ImU32 col = ImGui::GetColorU32(c);
        game::Vec3 p[8] = {{lo.x, lo.y, lo.z}, {hi.x, lo.y, lo.z}, {hi.x, lo.y, hi.z}, {lo.x, lo.y, hi.z},
                           {lo.x, hi.y, lo.z}, {hi.x, hi.y, lo.z}, {hi.x, hi.y, hi.z}, {lo.x, hi.y, hi.z}};
        if (style_.i == 1) {
            ImVec2 a{FLT_MAX, FLT_MAX}, b{-FLT_MAX, -FLT_MAX};
            int seen = 0;
            for (auto& v : p)
                if (auto s = game::project(v)) {
                    a = {std::min(a.x, s->x), std::min(a.y, s->y)};
                    b = {std::max(b.x, s->x), std::max(b.y, s->y)};
                    seen++;
                }
            if (seen == 8) dl->AddRect(a, b, col, 0.f, 0, w);
        } else {
            static const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
            for (auto& e : edges) {
                ImVec2 s0, s1;
                if (game::projectLine(p[e[0]], p[e[1]], s0, s1)) dl->AddLine(s0, s1, col, w);
            }
        }
        float cx = (lo.x + hi.x) * 0.5f, cz = (lo.z + hi.z) * 0.5f;
        // players see from 1.62 above their feet; other mobs from about 85 % of their height
        float eyeY = lo.y + grow + (other ? std::min(1.62f, (hi.y - lo.y - 2 * grow) * 0.85f) : 1.62f);
        if (eye_.b) {
            ImVec4 ec = eyeColor_.color;
            ec.w *= opacity_.f;
            ImVec2 s0, s1;
            if (game::projectLine({lo.x, eyeY, cz}, {hi.x, eyeY, cz}, s0, s1)) dl->AddLine(s0, s1, ImGui::GetColorU32(ec), w);
            if (game::projectLine({cx, eyeY, lo.z}, {cx, eyeY, hi.z}, s0, s1)) dl->AddLine(s0, s1, ImGui::GetColorU32(ec), w);
        }
        if (look_.b) {
            ImVec4 lc = lookColor_.color;
            lc.w *= opacity_.f;
            float ry = yaw * 0.0174533f, rp = pitch * 0.0174533f, len = lookLength_.f;
            game::Vec3 from{cx, eyeY, cz};
            game::Vec3 to{cx - std::sin(ry) * std::cos(rp) * len, eyeY - std::sin(rp) * len, cz + std::cos(ry) * std::cos(rp) * len};
            ImVec2 s0, s1;
            if (game::projectLine(from, to, s0, s1)) dl->AddLine(s0, s1, ImGui::GetColorU32(lc), w);
        }
    }

    Setting& style_ = choice("style", "Box style", {"3D box", "Flat (2D)"});
    Setting& opacity_ = slider("opacity", "Opacity", 1.f, 0.1f, 1.f, "%.2f");
    Setting& width_ = slider("width", "Line thickness", 2.f, 0.5f, 8.f, "%.1f");
    Setting& self_ = toggleSetting("self", "Show your own hitbox", false);
    Setting& rainbow_ = toggleSetting("rainbow", "Rainbow", false);
    Setting& eye_ = toggleSetting("eye", "Eye line", false);
    Setting& look_ = toggleSetting("look", "Look direction line", false);
    Setting& lookLength_ = slider("lookLength", "Look line length", 3.f, 0.5f, 10.f, "%.1f");
    Setting& java_ = toggleSetting("java", "Java-style size (+0.1)", false);
    Setting& javaKey_ = keySetting("javaKey", "Java size key", 0);
    Setting& color_ = colorSetting("color", "Color", {0.23f, 0.65f, 0.93f, 1.f});
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
        bow_.visible = block_.visible = [] { return need::have("UseState"); };
        sneak_.visible = sprint_.visible = [] { return need::have("MoveState"); };
    }

    void drawSettings() override {
        ImGui::Spacing();
        if (fx::available(fx::Id::Sensitivity)) return;
        if (gameinput::active()) widgets::hint("Works on the mouse movement the game reads. Bow, block, sneak and sprint values need game data.");
        else widgets::hint("Needs game data or the game's own input on this version.");
    }

    void onFrame() override {
        auto& p = game::state().player;
        float m = base_.f;
        if (ctx::zooming) m *= zoom_.f;
        if (p.usingItem && p.held().name == "bow") m *= bow_.f;
        if (p.blocking) m *= block_.f;
        if (p.sneaking) m *= sneak_.f;
        if (p.sprinting) m *= sprint_.f;
        if (fx::available(fx::Id::Sensitivity)) fx::scale(fx::Id::Sensitivity, m);
        else gameinput::scaleMouse(m);
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
        require(need::player, need::sigs({"LocalPlayer", "UseState"}));
    }

    void onFrame() override {
        auto& p = game::state().player;
        if (!p.usingItem || (p.held().name != "bow" && onlyBow_.b)) return;
        if (fx::available(fx::Id::Sensitivity)) fx::scale(fx::Id::Sensitivity, factor_.f);
        else gameinput::scaleMouse(factor_.f);
    }

private:
    Setting& factor_ = slider("factor", "Sensitivity", 0.7f, 0.1f, 2.f, "%.2fx");
    Setting& onlyBow_ = toggleSetting("onlyBow", "Only with a bow", true);
};

class SnapLook : public Module {
public:
    SnapLook()
        : Module("Snap Look", "A short look behind you on a key, without changing your aim.",
                 Category::Pvp, {"camera"}) {
        sub("Input");
        require(0, {fx::sig(fx::Id::Perspective)});
    }

    void onKey(KeyEvent& ev) override {
        if (ev.vk != key_.i || !key_.i || ev.repeat) return;
        if (mode_.i == 0) active_ = ev.down;
        else if (ev.down) active_ = !active_;
    }

    // the front view puts the camera ahead of the player looking back, so it shows what is behind while the
    // player keeps facing forward
    void onFrame() override {
        if (active_) fx::setInt(fx::Id::Perspective, 2);
    }

    void onDisable() override { active_ = false; }

private:
    Setting& key_ = keySetting("snapKey", "Look key", 'V');
    Setting& mode_ = choice("mode", "Mode", {"Hold", "Toggle"});
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

    void onFrame() override {
        for (int axis = 0; axis < 2; axis++)
            for (int side = 0; side < 2; side++)
                if (cut_[axis][side]) gameinput::drop(key(axis, side));
    }

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
