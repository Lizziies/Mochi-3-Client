#pragma once

#include "modules/common/Colors.hpp"
#include "modules/common/GameHud.hpp"
#include "modules/common/Needs.hpp"
#include "modules/common/Text.hpp"
#include "modules/network/Probe.hpp"
#include "render/Ui.hpp"

#include <algorithm>
#include <format>

class ReachCounter : public GameText {
public:
    ReachCounter()
        : GameText("Reach Counter", "Zeigt die Distanz deines letzten Treffers, als Durchschnitt oder Bestwert.", need::combat,
                   need::sigs({"LocalPlayer", "AttackEntity"}), {"hud-self"}, {0.01f, 0.40f}) {
        sub("Kampf-Anzeigen");
        window_.visible = [this] { return mode_.i == 1; };
        warn_.visible = [this] { return colored_.b; };
    }

    void onRender(ImDrawList* dl) override {
        auto& c = game::state().combat;
        if (idle_.f > 0.f && ui::time() - c.lastHitAt > idle_.f && !gui::editingHud()) return;
        GameText::onRender(dl);
    }

protected:
    std::string label() const override { return showLabel_.b ? "Reach" : ""; }

    std::string value() override {
        auto& c = game::state().combat;
        if (!c.reachCount) return "–";
        float v = c.lastReach;
        if (mode_.i == 1) {
            int n = std::min(c.reachCount, std::min<int>(window_.i, int(c.reaches.size())));
            float sum = 0.f;
            for (int k = 0; k < n; k++) sum += c.reaches[size_t((c.reachCount - 1 - k) % int(c.reaches.size()))];
            v = sum / float(n);
        } else if (mode_.i == 2) {
            v = c.bestReach;
        }
        shown_ = v;
        return text::num(v, decimals_.i) + (unit_.b ? " Blöcke" : "");
    }

    ImU32 valueColor() const override {
        if (!colored_.b) return textColor();
        return ImGui::GetColorU32(rampColor(shown_, 2.5f, warn_.f, good_.color, mid_.color, bad_.color));
    }

private:
    Setting& mode_ = choice("mode", "Wert", {"Letzter Treffer", "Durchschnitt", "Bestwert"});
    Setting& window_ = intSlider("window", "Treffer im Durchschnitt", 5, 2, 10);
    Setting& decimals_ = intSlider("decimals", "Nachkommastellen", 2, 0, 3);
    Setting& unit_ = toggleSetting("unit", "Einheit anzeigen", false);
    Setting& showLabel_ = toggleSetting("label", "Beschriftung", true);
    Setting& idle_ = slider("idle", "Ausblenden nach (s, 0 = nie)", 0.f, 0.f, 30.f, "%.0f s");
    Setting& colored_ = toggleSetting("colored", "Farbe nach Wert", false);
    Setting& warn_ = slider("warn", "Rot ab (Blöcke)", 3.2f, 2.6f, 4.f, "%.1f");
    Setting& good_ = colorSetting("good", "Farbe niedrig", {0.55f, 0.91f, 0.69f, 1.f});
    Setting& mid_ = colorSetting("mid", "Farbe mittel", {1.f, 0.82f, 0.49f, 1.f});
    Setting& bad_ = colorSetting("bad", "Farbe hoch", {1.f, 0.4f, 0.45f, 1.f});
    mutable float shown_ = 0.f;
};

class OpponentReach : public GameText {
public:
    OpponentReach()
        : GameText("Opponent Reach", "Zeigt die Distanz, aus der dich der Gegner zuletzt getroffen hat.", need::combat,
                   need::sigs({"LocalPlayer", "AttackEntity"}), {"info-others"}, {0.01f, 0.44f}) {
        sub("Kampf-Anzeigen");
    }

protected:
    std::string label() const override { return "Gegner-Reach"; }

    std::string value() override {
        float r = game::state().combat.opponentReach;
        return r > 0.f ? text::num(r, decimals_.i) : "–";
    }

private:
    Setting& decimals_ = intSlider("decimals", "Nachkommastellen", 2, 0, 3);
};

class ComboCounter : public GameText {
public:
    ComboCounter()
        : GameText("Combo Counter", "Zählt deine Treffer in Folge, bis dich der Gegner trifft.", need::combat,
                   need::sigs({"LocalPlayer", "AttackEntity"}), {"hud-self"}, {0.01f, 0.48f}) {
        sub("Kampf-Anzeigen");
        timeout_.visible = [this] { return expire_.b; };
    }

    void onFrame() override {
        auto& c = game::state().combat;
        if (c.combo > last_) pulse_ = 1.f;
        last_ = c.combo;
        pulse_ = std::max(0.f, pulse_ - ui::dt() * 4.f);
        if (expire_.b && c.combo > 0 && ui::time() - c.lastHitAt > timeout_.f) {
            game::resetCombo();
            last_ = 0;
        }
    }

protected:
    std::string label() const override { return showLabel_.b ? "Combo" : ""; }

    std::string value() override {
        auto& c = game::state().combat;
        std::string v = std::to_string(c.combo);
        if (best_.b) v += std::format("  ·  Best {}", c.bestCombo);
        return v;
    }

    ImU32 valueColor() const override {
        if (!flash_.b) return textColor();
        return ImGui::GetColorU32(theme::mix(textColor_.color, flashColor_.color, pulse_));
    }

private:
    Setting& showLabel_ = toggleSetting("label", "Beschriftung", true);
    Setting& best_ = toggleSetting("best", "Rekord anzeigen", true);
    Setting& flash_ = toggleSetting("flash", "Aufleuchten bei neuem Treffer", true);
    Setting& flashColor_ = colorSetting("flashColor", "Leuchtfarbe", {1.f, 0.49f, 0.71f, 1.f});
    Setting& expire_ = toggleSetting("expire", "Combo läuft ab", false);
    Setting& timeout_ = slider("timeout", "Ablauf nach (s)", 3.f, 1.f, 10.f, "%.1f s");
    int last_ = 0;
    float pulse_ = 0.f;
};

class HitCounter : public GameText {
public:
    HitCounter()
        : GameText("Hit Counter", "Treffer, Schwünge und Trefferquote der aktuellen Runde.", need::combat,
                   need::sigs({"LocalPlayer", "AttackEntity"}), {"hud-self"}, {0.01f, 0.52f}) {
        sub("Kampf-Anzeigen");
    }

    void onKey(KeyEvent& ev) override {
        if (ev.down && !ev.repeat && ev.vk == reset_.i && reset_.i) game::resetCombat();
    }

protected:
    std::string label() const override { return "Treffer"; }

    std::string value() override {
        auto& c = game::state().combat;
        std::string out = std::to_string(c.hits);
        if (swings_.b) out += std::format(" / {}", c.swings);
        if (accuracy_.b && c.swings > 0) out += std::format("  ·  {:.0f}%", 100.f * c.hits / c.swings);
        if (crits_.b && c.hits > 0) out += std::format("  ·  Crit {:.0f}%", 100.f * c.crits / c.hits);
        return out;
    }

private:
    Setting& swings_ = toggleSetting("swings", "Schwünge anzeigen", true);
    Setting& accuracy_ = toggleSetting("accuracy", "Trefferquote", true);
    Setting& crits_ = toggleSetting("crits", "Crit-Quote", false);
    Setting& reset_ = keySetting("reset", "Zurücksetzen", 0);
};

class HitPing : public GameText {
public:
    HitPing()
        : GameText("Hit Ping", "Zeigt deinen Ping im Moment deines letzten Treffers.", need::combat,
                   need::sigs({"LocalPlayer", "AttackEntity"}), {"info-others"}, {0.01f, 0.56f}) {
        sub("Kampf-Anzeigen");
    }

    void onEnable() override { probe::use(true); }
    void onDisable() override { probe::use(false); }

    void onFrame() override {
        for (auto& e : game::events()) {
            if (e.kind != game::EventKind::Hit) continue;
            auto s = probe::snapshot();
            ping_ = s.received ? (s.last >= 0.f ? s.last : s.avg) : float(game::state().world.ping);
            seen_ = true;
        }
    }

protected:
    std::string label() const override { return "Hit-Ping"; }

    std::string value() override { return seen_ ? std::format("{:.0f} ms", ping_) : "–"; }

private:
    float ping_ = 0.f;
    bool seen_ = false;
};

class SessionStats : public GameText {
public:
    SessionStats()
        : GameText("Session Stats", "Kills, Tode, K/D und Kill-Serie dieser Sitzung.", need::combat,
                   need::sigs({"LocalPlayer", "AttackEntity", "ChatEvents"}), {"hud-self"}, {0.01f, 0.60f}) {
        sub("Kampf-Anzeigen");
    }

    void onKey(KeyEvent& ev) override {
        if (ev.down && !ev.repeat && ev.vk == reset_.i && reset_.i) game::resetCombat();
    }

protected:
    std::string value() override {
        auto& c = game::state().combat;
        std::string out;
        if (kills_.b) out += std::format("K {}  ", c.kills);
        if (deaths_.b) out += std::format("T {}  ", c.deaths);
        if (kd_.b) out += std::format("K/D {:.2f}  ", c.deaths ? float(c.kills) / c.deaths : float(c.kills));
        if (streak_.b) out += std::format("Serie {} (Best {})", c.streak, c.bestStreak);
        while (!out.empty() && out.back() == ' ') out.pop_back();
        return out.empty() ? "–" : out;
    }

private:
    Setting& kills_ = toggleSetting("kills", "Kills", true);
    Setting& deaths_ = toggleSetting("deaths", "Tode", true);
    Setting& kd_ = toggleSetting("kd", "K/D", true);
    Setting& streak_ = toggleSetting("streak", "Serie", true);
    Setting& reset_ = keySetting("reset", "Zurücksetzen", 0);
};

class HitInfo : public GameText {
public:
    HitInfo()
        : GameText("Hit Info", "Zeigt, ob dein letzter Schlag ein kritischer Treffer war, und die Crit-Quote.", need::combat,
                   need::sigs({"LocalPlayer", "AttackEntity"}), {"hud-self"}, {0.01f, 0.64f}) {
        sub("Kampf-Anzeigen");
    }

protected:
    std::string value() override {
        auto& c = game::state().combat;
        if (!c.hits) return "–";
        std::string out = c.lastCrit ? "Kritisch!" : "Normal";
        if (rate_.b) out += std::format("  ·  {:.0f}% Crit", 100.f * c.crits / c.hits);
        if (damage_.b) out += std::format("  ·  {:.0f} Schaden", c.damageDealt);
        return out;
    }

    ImU32 valueColor() const override {
        return ImGui::GetColorU32(game::state().combat.lastCrit ? critColor_.color : textColor_.color);
    }

private:
    Setting& rate_ = toggleSetting("rate", "Crit-Quote", true);
    Setting& damage_ = toggleSetting("damage", "Gesamtschaden", false);
    Setting& critColor_ = colorSetting("critColor", "Farbe bei Crit", {1.f, 0.49f, 0.71f, 1.f});
};

class EntityCounter : public GameText {
public:
    EntityCounter()
        : GameText("Entity Counter", "Zählt Entities und Spieler in deiner Umgebung.", need::world, need::sigs({"Level", "EntityList"}),
                   {"info-others"}, {0.01f, 0.68f}) {
        sub("Kampf-Anzeigen");
    }

protected:
    std::string label() const override { return "Entities"; }

    std::string value() override {
        auto& w = game::state().world;
        return players_.b ? std::format("{}  ·  {} Spieler", w.entities, w.players) : std::to_string(w.entities);
    }

private:
    Setting& players_ = toggleSetting("players", "Spieler zusätzlich", true);
};
