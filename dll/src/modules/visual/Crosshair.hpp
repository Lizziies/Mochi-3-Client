#pragma once

#include "gui/Gui.hpp"
#include "hook/Input.hpp"
#include "modules/Module.hpp"
#include "render/Draw.hpp"

#include <cmath>

class Crosshair : public Module {
public:
    Crosshair()
        : Module("Custom Crosshair", "Eigenes Fadenkreuz: Kreuz, Punkt, Kreis oder Herz, mit Farbe, Umriss und Klick-Effekt.",
                 Category::Visual, {"cosmetic"}) {}

    void onRender(ImDrawList* dl) override {
        if (gui::open() || gui::editingHud()) return;
        auto ds = ImGui::GetIO().DisplaySize;
        ImVec2 c{std::floor(ds.x * 0.5f) + 0.5f, std::floor(ds.y * 0.5f) + 0.5f};

        bool clicking = input::down(VK_LBUTTON);
        pulse_ = draw::approach(pulse_, clicking ? 1.f : 0.f, clicking ? 40.f : 10.f);
        float size = size_.f * (1.f + (clickPulse_.b ? pulse_ * 0.25f : 0.f));
        ImVec4 col = clickColor_.b ? lerp(color_.color, activeColor_.color, pulse_) : color_.color;

        if (outline_.b) shape(dl, c, size, thickness_.f + 2.f, IM_COL32(0, 0, 0, int(200 * col.w)), true);
        shape(dl, c, size, thickness_.f, ImGui::GetColorU32(col), false);
    }

private:
    static ImVec4 lerp(ImVec4 a, ImVec4 b, float t) {
        return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
    }

    void cross(ImDrawList* dl, ImVec2 c, float size, float th, ImU32 col) {
        float g = gap_.f;
        dl->AddLine({c.x - size, c.y}, {c.x - g, c.y}, col, th);
        dl->AddLine({c.x + g, c.y}, {c.x + size, c.y}, col, th);
        dl->AddLine({c.x, c.y - size}, {c.x, c.y - g}, col, th);
        dl->AddLine({c.x, c.y + g}, {c.x, c.y + size}, col, th);
    }

    void shape(ImDrawList* dl, ImVec2 c, float size, float th, ImU32 col, bool outline) {
        switch (style_.i) {
        case 0: cross(dl, c, size, th, col); break;
        case 1: dl->AddCircleFilled(c, th * 1.2f + (outline ? 1 : 0), col); break;
        case 2: dl->AddCircle(c, size * 0.6f, col, 32, th); break;
        case 3:
            cross(dl, c, size, th, col);
            dl->AddCircleFilled(c, th * 0.9f + (outline ? 1 : 0), col);
            break;
        case 4: draw::heart(dl, c, size * 1.4f + (outline ? 2 : 0), col); break;
        }
    }

    Setting& style_ = choice("style", "Form", {"Kreuz", "Punkt", "Kreis", "Kreuz + Punkt", "Herz"});
    Setting& size_ = slider("size", "Größe", 8.f, 2.f, 30.f, "%.0f");
    Setting& gap_ = slider("gap", "Abstand", 2.f, 0.f, 12.f, "%.0f");
    Setting& thickness_ = slider("thickness", "Dicke", 2.f, 1.f, 6.f, "%.1f");
    Setting& color_ = colorSetting("color", "Farbe", {1.f, 1.f, 1.f, 0.95f});
    Setting& outline_ = toggleSetting("outline", "Umriss", true);
    Setting& clickColor_ = toggleSetting("clickColor", "Farbe beim Klicken", true);
    Setting& activeColor_ = colorSetting("activeColor", "Klickfarbe", {1.f, 0.49f, 0.71f, 1.f});
    Setting& clickPulse_ = toggleSetting("pulse", "Pulsieren beim Klicken", true);
    float pulse_ = 0;
};
