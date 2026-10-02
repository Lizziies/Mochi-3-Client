#pragma once

#include "core/Config.hpp"
#include "gui/Gui.hpp"
#include "modules/HudModule.hpp"
#include "modules/common/Layout.hpp"
#include "render/Fonts.hpp"
#include "sdk/Effects.hpp"

#include <cmath>

class MovableElement : public HudModule {
public:
    MovableElement(std::string name, std::string description, fx::Id channel, ImVec2 defaultPos)
        : HudModule(std::move(name), std::move(description), {"cosmetic"}, defaultPos), channel_(channel) {
        sub("HUD parts");
        background_.b = false;
        requireAny({fx::sig(channel)});
        for (auto& st : settings())
            if (st.style) st.hidden = true;
    }

    void onFrame() override {
        auto ds = ImGui::GetIO().DisplaySize;
        if (ds.x <= 0.f) return;
        float k = layout::guiScale(ds, guiScale_.f);
        ImVec2 home = homeAt(ds, k);
        if (!placed_.b) {
            setPosition(home);
            placed_.b = true;
            config::markDirty();
        }
        ImVec2 p = position();
        float dx = p.x - home.x, dy = p.y - home.y;
        if (std::fabs(dx) < 0.5f && std::fabs(dy) < 0.5f) return;
        fx::out(channel_, {dx, dy});
    }

    void onRender(ImDrawList* dl) override {
        if (!gui::editingHud()) return;
        HudModule::onRender(dl);
    }

protected:
    bool autoPlace() const override { return false; }
    virtual ImVec2 homeAt(ImVec2 display, float guiScale) const = 0;
    virtual ImVec2 sizeAt(float guiScale) const = 0;

    ImVec2 content(ImDrawList* dl, ImVec2 o, float) override {
        auto ds = ImGui::GetIO().DisplaySize;
        ImVec2 size = sizeAt(layout::guiScale(ds, guiScale_.f));
        if (gui::editingHud()) {
            dl->AddRectFilled(o, o + size, IM_COL32(59, 167, 236, 40), 4.f);
            dl->AddRect(o, o + size, IM_COL32(59, 167, 236, 200), 4.f, 0, 1.5f);
            ImVec2 ts = fonts::hud()->CalcTextSizeA(fonts::hudSize(), FLT_MAX, 0.f, name().c_str());
            dl->AddText(fonts::hud(), fonts::hudSize(), o + (size - ts) * 0.5f, IM_COL32(255, 235, 245, 230), name().c_str());
        }
        return size;
    }

private:
    fx::Id channel_;
    Setting& guiScale_ = slider("guiScale", "GUI scale of the game (0 = automatic)", 0.f, 0.f, 6.f, "%.0f");
};

class MovableHotbar : public MovableElement {
public:
    MovableHotbar() : MovableElement("Movable Hotbar", "Move the game's own hotbar anywhere on the screen. Drag the frame in the HUD editor.", fx::Id::HotbarOffset, {0.3f, 0.93f}) {}

protected:
    ImVec2 homeAt(ImVec2 ds, float k) const override { return {(ds.x - 182.f * k) * 0.5f, ds.y - 22.f * k}; }
    ImVec2 sizeAt(float k) const override { return {182.f * k, 22.f * k}; }
};

class MovableTitle : public MovableElement {
public:
    MovableTitle() : MovableElement("Movable Title", "Move the big title text of the game, for example away from the middle of the screen.", fx::Id::TitleOffset, {0.4f, 0.33f}) {}

protected:
    ImVec2 homeAt(ImVec2 ds, float k) const override { return {(ds.x - 160.f * k) * 0.5f, ds.y * 0.33f}; }
    ImVec2 sizeAt(float k) const override { return {160.f * k, 36.f * k}; }
};

class MovableBossbar : public MovableElement {
public:
    MovableBossbar() : MovableElement("Movable Bossbar", "Move the boss bar at the top of the screen. Drag the frame in the HUD editor.", fx::Id::BossbarOffset, {0.3f, 0.02f}) {}

protected:
    ImVec2 homeAt(ImVec2 ds, float k) const override { return {(ds.x - 182.f * k) * 0.5f, 4.f * k}; }
    ImVec2 sizeAt(float k) const override { return {182.f * k, 20.f * k}; }
};
