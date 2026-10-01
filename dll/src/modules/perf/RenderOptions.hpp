#pragma once

#include "modules/Module.hpp"
#include "sdk/Effects.hpp"

class RenderOptions : public Module {
public:
    RenderOptions()
        : Module("Render Options", "Schaltet teure Grafik wie Wolken, Partikel und Schatten einzeln ab.",
                 Category::Performance, {"performance"}) {
        sub("Grafik");
        requireAny({fx::sig(fx::Id::Clouds), fx::sig(fx::Id::Particles), fx::sig(fx::Id::BlockEntities), fx::sig(fx::Id::Shadows),
                    fx::sig(fx::Id::Sky), fx::sig(fx::Id::Fog), fx::sig(fx::Id::Vignette), fx::sig(fx::Id::Rain)});
        for (auto* s : {&clouds_, &particles_, &blockEntities_, &shadows_, &weather_, &fog_, &sky_, &vignette_})
            s->visible = [this] { return preset_.i == 0; };
    }

    void onFrame() override {
        bool clouds = clouds_.b, particles = particles_.b, blocks = blockEntities_.b, shadows = shadows_.b, weather = weather_.b, fog = fog_.b,
             sky = sky_.b, vignette = vignette_.b;
        if (preset_.i == 1) {
            clouds = particles = blocks = shadows = weather = fog = vignette = true;
            sky = false;
        } else if (preset_.i == 2) {
            clouds = shadows = weather = vignette = true;
            particles = blocks = fog = sky = false;
        }
        if (clouds) fx::force(fx::Id::Clouds, false);
        if (particles) fx::force(fx::Id::Particles, false);
        if (blocks) fx::force(fx::Id::BlockEntities, false);
        if (shadows) fx::force(fx::Id::Shadows, false);
        if (weather) {
            fx::set(fx::Id::Rain, 0.f);
            fx::set(fx::Id::Thunder, 0.f);
        }
        if (fog) fx::force(fx::Id::Fog, false);
        if (sky) fx::force(fx::Id::Sky, false);
        if (vignette) fx::force(fx::Id::Vignette, false);
    }

    void drawSettings() override {
        ImGui::Spacing();
        ImGui::TextDisabled("Auf dieser Version verfügbar:");
        for (auto id : {fx::Id::Clouds, fx::Id::Particles, fx::Id::BlockEntities, fx::Id::Shadows, fx::Id::Sky, fx::Id::Fog, fx::Id::Vignette, fx::Id::Rain}) {
            bool ok = fx::available(id);
            ImGui::TextColored(ok ? ImVec4(0.55f, 0.91f, 0.69f, 1.f) : ImVec4(0.6f, 0.5f, 0.6f, 1.f), "%s %s", ok ? "●" : "○", fx::info(id).label);
        }
    }

private:
    Setting& preset_ = choice("preset", "Vorlage", {"Eigene Auswahl", "PvP Max FPS", "Ausgewogen"});
    Setting& clouds_ = toggleSetting("clouds", "Wolken aus", true);
    Setting& particles_ = toggleSetting("particles", "Partikel aus", false);
    Setting& blockEntities_ = toggleSetting("blockEntities", "Block-Entities aus (Truhen, Schilder)", false);
    Setting& shadows_ = toggleSetting("shadows", "Schatten aus", true);
    Setting& weather_ = toggleSetting("weather", "Wetter-Effekte aus", false);
    Setting& fog_ = toggleSetting("fog", "Nebel aus", false);
    Setting& sky_ = toggleSetting("sky", "Himmel aus", false);
    Setting& vignette_ = toggleSetting("vignette", "Vignette aus", true);
};
