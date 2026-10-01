#pragma once

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
        : Module("Hurt Color", "Ändert die rote Farbe, wenn du oder ein Gegner Schaden nehmen.", Category::Pvp, {"cosmetic"}) {
        sub("Treffer-Visuals");
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
    Setting& color_ = colorSetting("color", "Farbe", {1.f, 0.35f, 0.6f, 1.f});
    Setting& intensity_ = slider("intensity", "Stärke", 0.6f, 0.05f, 1.f, "%.2f");
    Setting& rainbow_ = toggleSetting("rainbow", "Regenbogen", false);
    Setting& speed_ = slider("speed", "Tempo", 1.f, 0.1f, 5.f, "%.1f");
};

class GlintColor : public Module {
public:
    GlintColor()
        : Module("Glint Color", "Ändert die Farbe des Verzauberungs-Glanzes auf Items und Rüstung.", Category::Pvp, {"cosmetic"}) {
        sub("Treffer-Visuals");
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
    Setting& color_ = colorSetting("color", "Farbe", {1.f, 0.49f, 0.71f, 1.f});
    Setting& rainbow_ = toggleSetting("rainbow", "Regenbogen", false);
    Setting& speed_ = slider("speed", "Tempo", 1.f, 0.1f, 5.f, "%.1f");
};

class Hitbox : public Module {
public:
    Hitbox()
        : Module("Hitbox", "Zeigt die Hitboxen mit dem eigenen Zeichenweg des Spiels, also nicht durch Wände. Farbe einstellbar.",
                 Category::Pvp, {"info-others"}) {
        sub("Treffer-Visuals");
        require(0, {fx::sig(fx::Id::Hitbox)});
        color_.visible = [this] { return !rainbow_.b; };
    }

    void onFrame() override {
        fx::force(fx::Id::Hitbox, true);
        ImVec4 c = color_.color;
        if (rainbow_.b) {
            float r, g, b;
            ImGui::ColorConvertHSVtoRGB(std::fmod(float(ui::time()) * 0.3f, 1.f), 0.7f, 1.f, r, g, b);
            c = {r, g, b, 1.f};
        }
        fx::out(fx::Id::HitboxColor, {c.x, c.y, c.z, c.w});
    }

private:
    Setting& color_ = colorSetting("color", "Farbe", {1.f, 0.49f, 0.71f, 1.f});
    Setting& rainbow_ = toggleSetting("rainbow", "Regenbogen", false);
};

class LowFire : public Module {
public:
    LowFire()
        : Module("Low Fire", "Senkt das Feuer-Overlay am unteren Bildrand oder blendet es ganz aus.", Category::Pvp, {"cosmetic"}) {
        sub("Treffer-Visuals");
        require(0, {fx::sig(fx::Id::FireHeight)});
    }

    void onFrame() override { fx::scale(fx::Id::FireHeight, 1.f - amount_.f); }

private:
    Setting& amount_ = slider("amount", "Absenken", 0.6f, 0.f, 1.f, "%.2f");
};

class ParticleMultiplier : public Module {
public:
    ParticleMultiplier()
        : Module("Particle Multiplier", "Mehr oder weniger Partikel. Weniger hilft den FPS, mehr sieht schöner aus.", Category::Pvp, {"cosmetic"}) {
        sub("Treffer-Visuals");
        require(0, {fx::sig(fx::Id::ParticleScale)});
    }

    void onFrame() override { fx::set(fx::Id::ParticleScale, amount_.f); }

private:
    Setting& amount_ = slider("amount", "Menge", 0.5f, 0.f, 4.f, "%.2fx");
};

class SensMultiplier : public Module {
public:
    SensMultiplier()
        : Module("Sens Multiplier", "Eigene Empfindlichkeit beim Zoomen, Bogenspannen, Blocken, Schleichen und Sprinten.",
                 Category::Pvp, {"input"}) {
        sub("Eingabe");
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
    Setting& base_ = slider("base", "Grundwert", 1.f, 0.2f, 3.f, "%.2fx");
    Setting& zoom_ = slider("zoom", "Beim Zoomen", 0.6f, 0.1f, 2.f, "%.2fx");
    Setting& bow_ = slider("bow", "Beim Bogenspannen", 1.f, 0.1f, 2.f, "%.2fx");
    Setting& block_ = slider("block", "Beim Blocken", 1.f, 0.1f, 2.f, "%.2fx");
    Setting& sneak_ = slider("sneak", "Beim Schleichen", 1.f, 0.1f, 2.f, "%.2fx");
    Setting& sprint_ = slider("sprint", "Beim Sprinten", 1.f, 0.1f, 2.f, "%.2fx");
};

class BowSensitivity : public Module {
public:
    BowSensitivity()
        : Module("Bow Sensitivity", "Eigene Empfindlichkeit, solange du den Bogen spannst, für präziseres Zielen.", Category::Pvp, {"input"}) {
        sub("Eingabe");
        require(need::player, {fx::sig(fx::Id::Sensitivity), "LocalPlayer"});
    }

    void onFrame() override {
        auto& p = game::state().player;
        if (p.usingItem && (p.held().name == "bow" || !onlyBow_.b)) fx::scale(fx::Id::Sensitivity, factor_.f);
    }

private:
    Setting& factor_ = slider("factor", "Empfindlichkeit", 0.7f, 0.1f, 2.f, "%.2fx");
    Setting& onlyBow_ = toggleSetting("onlyBow", "Nur mit Bogen", true);
};

class SnapLook : public Module {
public:
    SnapLook()
        : Module("Snap Look", "Kurzer Blick nach hinten oder zur Seite per Taste, ohne die Blickrichtung zu ändern.",
                 Category::Pvp, {"camera"}) {
        sub("Eingabe");
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
    Setting& key_ = keySetting("snapKey", "Blick-Taste", 'V');
    Setting& mode_ = choice("mode", "Modus", {"Halten", "Umschalten"});
    Setting& dir_ = choice("dir", "Richtung", {"Hinten", "Links", "Rechts"});
    Setting& keepPitch_ = toggleSetting("keepPitch", "Neigung beibehalten", true);
    bool active_ = false;
};

class NullMovement : public Module {
public:
    NullMovement()
        : Module("Null Movement",
                 "Gegenläufige Richtungstasten heben sich nicht auf. Auf vielen Servern verboten.",
                 Category::Pvp, {"input"}) {
        sub("Eingabe");
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

    Setting& mode_ = choice("mode", "Verhalten", {"Zuletzt gedrückte Taste gewinnt", "Beide neutralisieren"});
    Setting& horizontal_ = toggleSetting("horizontal", "Links / Rechts", true);
    Setting& vertical_ = toggleSetting("vertical", "Vor / Zurück", false);
    Setting& left_ = keySetting("left", "Links", 'A');
    Setting& right_ = keySetting("right", "Rechts", 'D');
    Setting& forward_ = keySetting("forward", "Vor", 'W');
    Setting& back_ = keySetting("back", "Zurück", 'S');
    bool held_[2][2]{};
    bool cut_[2][2]{};
};

class ItemUseDelayFix : public Module {
public:
    ItemUseDelayFix()
        : Module("Item Use Delay Fix", "Verkürzt die Wartezeit zwischen Item-Nutzungen. Auf vielen Servern verboten.",
                 Category::Pvp, {"timing"}) {
        sub("Eingabe");
        markRisky();
        require(0, {fx::sig(fx::Id::UseDelay)});
    }

    void onFrame() override { fx::scale(fx::Id::UseDelay, factor_.f); }

private:
    Setting& factor_ = slider("factor", "Restliche Wartezeit", 0.5f, 0.f, 1.f, "%.2f");
};

class FasterInventory : public Module {
public:
    FasterInventory()
        : Module("Faster Inventory", "Schnelleres Bewegen von Items im Inventar. Auf vielen Servern verboten.",
                 Category::Pvp, {"timing"}) {
        sub("Eingabe");
        markRisky();
        require(0, {fx::sig(fx::Id::InventoryDelay)});
    }

    void onFrame() override { fx::scale(fx::Id::InventoryDelay, factor_.f); }

private:
    Setting& factor_ = slider("factor", "Restliche Wartezeit", 0.5f, 0.f, 1.f, "%.2f");
};

class InstaHurtAnimation : public Module {
public:
    InstaHurtAnimation()
        : Module("Insta Hurt Animation", "Treffer-Animation sofort statt verzögert. Auf vielen Servern verboten.",
                 Category::Pvp, {"timing"}) {
        sub("Eingabe");
        markRisky();
        require(0, {fx::sig(fx::Id::HurtAnim)});
    }

    void onFrame() override { fx::force(fx::Id::HurtAnim, true); }
};
