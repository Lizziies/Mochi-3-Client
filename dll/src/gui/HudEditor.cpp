#include "HudEditor.hpp"
#include "Gui.hpp"
#include "Theme.hpp"
#include "modules/HudModule.hpp"
#include "modules/Manager.hpp"
#include "render/Draw.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"

#include <imgui.h>

#include <cmath>

namespace hudeditor {

static HudModule* dragging = nullptr;
static ImVec2 grabOffset{0, 0};

static void dashedRect(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 col, float thick, float dash) {
    auto line = [&](ImVec2 p, ImVec2 q) {
        ImVec2 d = q - p;
        float len = std::sqrt(d.x * d.x + d.y * d.y);
        if (len <= 0) return;
        ImVec2 dir = d / len;
        for (float t = 0; t < len; t += dash * 2) {
            float e = std::min(t + dash, len);
            dl->AddLine(p + dir * t, p + dir * e, col, thick);
        }
    };
    line(a, {b.x, a.y});
    line({b.x, a.y}, b);
    line(b, {a.x, b.y});
    line({a.x, b.y}, a);
}

static float snap(float v, float size, float screen, float threshold, bool& hitCenter, bool& hitEdge) {
    float center = (screen - size) * 0.5f;
    if (std::fabs(v - center) < threshold) {
        hitCenter = true;
        return center;
    }
    if (std::fabs(v) < threshold) {
        hitEdge = true;
        return 0;
    }
    if (std::fabs(v + size - screen) < threshold) {
        hitEdge = true;
        return screen - size;
    }
    return v;
}

void draw() {
    auto& t = theme::current();
    float s = ui::scale();
    auto& io = ImGui::GetIO();
    auto ds = io.DisplaySize;
    auto* fg = ImGui::GetForegroundDrawList();
    auto* bg = ImGui::GetBackgroundDrawList();

    bg->AddRectFilled({0, 0}, ds, IM_COL32(10, 4, 12, 70));

    const char* help = "Ziehen = verschieben  ·  Mausrad = Größe  ·  Rechtsklick = Einstellungen  ·  ESC = fertig";
    ImVec2 hs = ImGui::CalcTextSize(help);
    ImVec2 hp{(ds.x - hs.x) * 0.5f - 16 * s, 18 * s};
    fg->AddRectFilled(hp, hp + hs + ImVec2(32 * s, 16 * s), theme::col(t.surface, 0.95f), 99.f);
    fg->AddText(hp + ImVec2(16 * s, 8 * s), theme::col(t.text), help);

    HudModule* hovered = nullptr;
    for (auto& m : modules::all()) {
        if (!m->enabled() || !m->isHud()) continue;
        auto* h = static_cast<HudModule*>(m.get());
        ImVec2 p = h->position(), sz = h->size();
        bool over = ImGui::IsMouseHoveringRect(p, p + sz, false);
        if (over) hovered = h;
        ImU32 col = theme::col(over || dragging == h ? t.accent : t.accent2, over ? 1.f : 0.6f);
        dashedRect(fg, p - ImVec2(3 * s, 3 * s), p + sz + ImVec2(3 * s, 3 * s), col, 1.5f * s, 5 * s);
        if (over && !dragging) {
            fg->AddText(fonts::regular(), 13 * s, p + ImVec2(0, -18 * s), theme::col(t.text), h->name().c_str());
        }
    }

    if (!dragging && hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        dragging = hovered;
        grabOffset = io.MousePos - hovered->position();
    }
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        gui::setEditingHud(false);
        gui::showModule(hovered);
        return;
    }
    if (hovered && io.MouseWheel != 0.f) hovered->setScale(hovered->scale() + io.MouseWheel * 0.08f);

    if (dragging) {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            dragging = nullptr;
            return;
        }
        ImVec2 target = io.MousePos - grabOffset;
        ImVec2 sz = dragging->size();
        bool cx = false, cy = false, ex = false, ey = false;
        if (!io.KeyShift) {
            target.x = snap(target.x, sz.x, ds.x, 8 * s, cx, ex);
            target.y = snap(target.y, sz.y, ds.y, 8 * s, cy, ey);
        }
        dragging->setPosition(target);
        ImU32 guide = theme::col(t.accent, 0.8f);
        if (cx) fg->AddLine({ds.x * 0.5f, 0}, {ds.x * 0.5f, ds.y}, guide, 1.f * s);
        if (cy) fg->AddLine({0, ds.y * 0.5f}, {ds.x, ds.y * 0.5f}, guide, 1.f * s);
    }
}

}
