#pragma once

#include "gui/Theme.hpp"
#include "modules/Manager.hpp"
#include "modules/Module.hpp"
#include "sdk/Effects.hpp"
#include "sdk/Game.hpp"
#include "sig/Sigs.hpp"

class SigStatus : public Module {
public:
    SigStatus()
        : Module("Game Support",
                 "Zeigt, welche Module auf dieser Version laufen. Hier gibt es auch Demo-Daten.",
                 Category::Performance, {"performance"}) {
        sub("Diagnose");
    }

    bool alwaysOn() const override { return true; }

    void onFrame() override {
        if (game::demo() == demo_.b) return;
        game::setDemo(demo_.b);
        modules::refreshSigs();
    }

    void drawSettings() override {
        auto& t = theme::current();
        auto st = sigs::stats();
        ImGui::Spacing();
        ImGui::TextDisabled("Version %s  ·  %d von %d Signaturen  ·  Quelle %s", st.gameVersion.c_str(), st.found, st.total, st.source.c_str());
        if (demo_.b) ImGui::TextColored(t.warn, "Demo-Daten aktiv: alle Spiel-Module zeigen simulierte Werte.");

        int missing = 0, ok = 0;
        for (auto& m : modules::all()) {
            if (m->sigs().empty()) continue;
            if (m->missingSigs().empty()) ok++;
            else missing++;
        }
        ImGui::Text("%d Spiel-Module bereit, %d warten auf Signaturen", ok, missing);

        if (ImGui::CollapsingHeader("Module und ihre Signaturen")) {
            for (auto& m : modules::all()) {
                if (m->sigs().empty()) continue;
                bool good = m->missingSigs().empty();
                ImGui::TextColored(good ? t.ok : t.textDim, "%s %s", good ? "●" : "○", m->name().c_str());
                if (!good) {
                    std::string list;
                    for (auto& s : m->missingSigs()) list += (list.empty() ? "" : ", ") + s;
                    ImGui::SameLine();
                    ImGui::TextDisabled("fehlt: %s", list.c_str());
                }
            }
        }
        if (ImGui::CollapsingHeader("Effekt-Kanäle")) {
            for (int i = 0; i < int(fx::Id::Count); i++) {
                auto id = fx::Id(i);
                auto r = fx::report(id);
                bool have = sigs::address(fx::info(id).sig) != 0;
                ImGui::TextColored(have ? t.ok : t.textDim, "%s %s", have ? "●" : "○", fx::info(id).sig);
                ImGui::SameLine();
                ImGui::TextDisabled("%s%s", fx::info(id).label, r.requested ? "  ·  angefordert" : "");
            }
        }
    }

private:
    Setting& demo_ = toggleSetting("demo", "Demo-Daten (simulierte Spielwerte)", false);
};
