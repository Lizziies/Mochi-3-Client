#pragma once

#include "gui/Notify.hpp"
#include "modules/Module.hpp"
#include "modules/common/GameHud.hpp"
#include "modules/common/Needs.hpp"
#include "modules/common/Text.hpp"
#include "render/Ui.hpp"
#include "sdk/Game.hpp"

#include <format>

class MatchSummary : public Module {
public:
    MatchSummary()
        : Module("Match Summary", "Zusammenfassung beim Verlassen eines Servers: Dauer, Treffer, Combo, K/D.", Category::Server,
                 {"hud-self"}) {
        sub("Statistik");
        require(need::combat, need::sigs({"LocalPlayer", "AttackEntity"}));
    }

    void onServer(const ServerEvent& ev) override {
        if (ev.joined) {
            start_ = ui::time();
            game::resetCombat();
            return;
        }
        auto& c = game::state().combat;
        std::string body = std::format("Dauer {}", text::clock(float(ui::time() - start_)));
        if (hits_.b) body += std::format("  ·  {} Treffer", c.hits);
        if (accuracy_.b && c.swings > 0) body += std::format(" ({:.0f}%)", 100.f * c.hits / c.swings);
        if (combo_.b) body += std::format("  ·  Combo {}", c.bestCombo);
        if (kd_.b) body += std::format("  ·  K/D {}/{}", c.kills, c.deaths);
        notify::push("Zusammenfassung: " + ev.name, body, notify::Kind::Info, 10.f);
    }

private:
    Setting& hits_ = toggleSetting("hits", "Treffer", true);
    Setting& accuracy_ = toggleSetting("accuracy", "Trefferquote", true);
    Setting& combo_ = toggleSetting("combo", "Combo-Rekord", true);
    Setting& kd_ = toggleSetting("kd", "Kills und Tode", true);
    double start_ = 0.0;
};
