#pragma once

#include "modules/Module.hpp"
#include "sdk/Effects.hpp"

class RenderOptions : public Module {
public:
    RenderOptions()
        : Module("Render Options", "Turns off expensive graphics like clouds, particles and shadows one by one.",
                 Category::Performance, {"performance"}) {
        sub("Graphics");
        requireAny({fx::sig(fx::Id::Clouds), fx::sig(fx::Id::Particles), fx::sig(fx::Id::BlockEntities), fx::sig(fx::Id::Shadows),
                    fx::sig(fx::Id::Sky), fx::sig(fx::Id::Fog), fx::sig(fx::Id::Vignette), fx::sig(fx::Id::Rain), fx::sig(fx::Id::RenderEntities),
                    fx::sig(fx::Id::RenderTerrain), fx::sig(fx::Id::HideHand), fx::sig(fx::Id::HideHud)});
        std::pair<Setting*, fx::Id> own[] = {{&clouds_, fx::Id::Clouds}, {&particles_, fx::Id::Particles}, {&blockEntities_, fx::Id::BlockEntities},
                                             {&shadows_, fx::Id::Shadows}, {&weather_, fx::Id::Rain}, {&fog_, fx::Id::Fog},
                                             {&sky_, fx::Id::Sky}, {&vignette_, fx::Id::Vignette}};
        for (auto [s, id] : own) s->visible = [this, id] { return preset_.i == 0 && fx::available(id); };
        entities_.visible = [] { return fx::available(fx::Id::RenderEntities); };
        terrain_.visible = [] { return fx::available(fx::Id::RenderTerrain); };
        hand_.visible = [] { return fx::available(fx::Id::HideHand); };
        hud_.visible = [] { return fx::available(fx::Id::HideHud); };
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
        if (entities_.b) fx::force(fx::Id::RenderEntities, false);
        if (terrain_.b) fx::force(fx::Id::RenderTerrain, false);
        if (hand_.b) fx::skip(fx::Id::HideHand);
        if (hud_.b) fx::skip(fx::Id::HideHud);
    }

    void drawSettings() override {
        ImGui::Spacing();
        ImGui::TextDisabled(i18n::tr("Available on this version:"));
        for (auto id : {fx::Id::Clouds, fx::Id::Particles, fx::Id::BlockEntities, fx::Id::Shadows, fx::Id::Sky, fx::Id::Fog, fx::Id::Vignette, fx::Id::Rain, fx::Id::RenderEntities, fx::Id::RenderTerrain, fx::Id::HideHand, fx::Id::HideHud}) {
            bool ok = fx::available(id);
            ImGui::TextColored(ok ? ImVec4(0.55f, 0.91f, 0.69f, 1.f) : ImVec4(0.6f, 0.5f, 0.6f, 1.f), "%s %s", ok ? "●" : "○", i18n::tr(fx::info(id).label));
        }
    }

private:
    Setting& preset_ = choice("preset", "Preset", {"Own selection", "PvP Max FPS", "Balanced"});
    Setting& clouds_ = toggleSetting("clouds", "Clouds off", true);
    Setting& particles_ = toggleSetting("particles", "Particles off", false);
    Setting& blockEntities_ = toggleSetting("blockEntities", "Block entities off (chests, signs)", false);
    Setting& shadows_ = toggleSetting("shadows", "Shadows off", true);
    Setting& weather_ = toggleSetting("weather", "Weather effects off", false);
    Setting& fog_ = toggleSetting("fog", "Fog off", false);
    Setting& sky_ = toggleSetting("sky", "Sky off", false);
    Setting& vignette_ = toggleSetting("vignette", "Vignette off", true);
    Setting& entities_ = toggleSetting("entities", "Entities off", false);
    Setting& terrain_ = toggleSetting("terrain", "Terrain off", false);
    Setting& hand_ = toggleSetting("hand", "Item in hand off", false);
    Setting& hud_ = toggleSetting("hud", "Original HUD off", false);
};
