#include "GuiInternal.hpp"
#include "I18n.hpp"
#include "Theme.hpp"
#include "core/Build.hpp"
#include "modules/Manager.hpp"
#include "modules/Tiers.hpp"
#include "render/Draw.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"
#include "server/Rules.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cmath>

namespace gui {

Category catOf(const Module& m) {
    int c = modules::displayCategory(m.name());
    return c >= 0 ? Category(c) : m.category();
}

ImVec4 catColor(int cat) {
    static const unsigned pal[] = {0xFF8FBF, 0xB69CFF, 0xFF8A80, 0x8BE9B0, 0xFFD27E, 0x7EC8FF, 0xFFB38A, 0xFF7EB6};
    unsigned c = cat >= 0 && cat < 8 ? pal[cat] : 0xFF7EB6;
    return {((c >> 16) & 255) / 255.f, ((c >> 8) & 255) / 255.f, (c & 255) / 255.f, 1.f};
}

void glyph(ImDrawList* dl, int kind, ImVec2 c, float r, ImU32 col) {
    float w = std::max(1.5f, r * 0.15f);
    switch (kind) {
    case -1:
        for (int i = 0; i < 4; i++) {
            float g = r * 0.38f;
            float x = c.x + ((i & 1) ? 1 : -1) * r * 0.52f, y = c.y + ((i & 2) ? 1 : -1) * r * 0.52f;
            dl->AddRectFilled({x - g, y - g}, {x + g, y + g}, col, g * 0.4f);
        }
        break;
    case 0:
        dl->AddRect({c.x - r, c.y - r * 0.72f}, {c.x + r, c.y + r * 0.72f}, col, r * 0.25f, 0, w);
        dl->AddLine({c.x - r * 0.55f, c.y - r * 0.15f}, {c.x + r * 0.15f, c.y - r * 0.15f}, col, w);
        dl->AddLine({c.x - r * 0.55f, c.y + r * 0.28f}, {c.x + r * 0.55f, c.y + r * 0.28f}, col, w);
        break;
    case 1:
        dl->AddEllipse(c, {r, r * 0.62f}, col, 0.f, 28, w);
        dl->AddCircleFilled(c, r * 0.3f, col, 16);
        break;
    case 2:
        dl->AddLine({c.x - r * 0.8f, c.y + r * 0.8f}, {c.x + r * 0.8f, c.y - r * 0.8f}, col, w * 1.3f);
        dl->AddLine({c.x + r * 0.8f, c.y + r * 0.8f}, {c.x - r * 0.8f, c.y - r * 0.8f}, col, w * 1.3f);
        dl->AddLine({c.x - r * 0.95f, c.y + r * 0.15f}, {c.x - r * 0.15f, c.y + r * 0.95f}, col, w);
        dl->AddLine({c.x + r * 0.95f, c.y + r * 0.15f}, {c.x + r * 0.15f, c.y + r * 0.95f}, col, w);
        break;
    case 3:
        for (int i = 0; i < 3; i++) {
            float y = c.y + (i - 1) * r * 0.62f;
            dl->AddLine({c.x - r, y}, {c.x + r, y}, col, w);
            dl->AddCircleFilled({c.x + (i == 1 ? 0.45f : -0.4f) * r, y}, r * 0.2f, col, 12);
        }
        break;
    case 4: {
        ImVec2 p[6] = {{c.x + r * 0.15f, c.y - r}, {c.x - r * 0.6f, c.y + r * 0.1f}, {c.x - r * 0.05f, c.y + r * 0.1f},
                       {c.x - r * 0.2f, c.y + r},  {c.x + r * 0.65f, c.y - r * 0.2f}, {c.x + r * 0.05f, c.y - r * 0.2f}};
        dl->AddConcavePolyFilled(p, 6, col);
        break;
    }
    case 5:
        for (int i = 0; i < 2; i++) {
            float y = c.y + (i ? 0.12f : -0.9f) * r;
            dl->AddRect({c.x - r, y}, {c.x + r, y + r * 0.78f}, col, r * 0.2f, 0, w);
            dl->AddCircleFilled({c.x - r * 0.55f, y + r * 0.39f}, r * 0.12f, col, 8);
        }
        break;
    case 6: draw::sparkle(dl, c, r, col); break;
    case 10:
        dl->AddCircle(c, r, col, 24, w);
        for (int i = 0; i < 3; i++) {
            float a = 3.6f + i * 1.15f;
            dl->AddCircleFilled({c.x + std::cos(a) * r * 0.5f, c.y + std::sin(a) * r * 0.5f}, r * 0.17f, col, 8);
        }
        break;
    case 11:
        dl->AddCircle({c.x, c.y - r * 0.38f}, r * 0.38f, col, 16, w);
        dl->PathArcTo({c.x, c.y + r * 0.95f}, r * 0.85f, 3.5f, 5.9f, 16);
        dl->PathStroke(col, 0, w);
        break;
    case 13:
        dl->AddRectFilled({c.x - r * 0.75f, c.y - r * 0.1f}, {c.x + r * 0.75f, c.y + r * 0.9f}, col, r * 0.2f);
        dl->PathArcTo({c.x, c.y - r * 0.1f}, r * 0.5f, 3.1416f, 6.2832f, 12);
        dl->PathStroke(col, 0, w);
        break;
    default:
        dl->AddCircle(c, r * 0.5f, col, 16, w);
        for (int i = 0; i < 8; i++) {
            float a = i * 0.7854f;
            dl->AddLine({c.x + std::cos(a) * r * 0.72f, c.y + std::sin(a) * r * 0.72f},
                        {c.x + std::cos(a) * r, c.y + std::sin(a) * r}, col, w * 1.4f);
        }
        break;
    }
}

std::string fitText(ImFont* font, float size, std::string text, float maxW) {
    auto width = [&](const std::string& t) { return font->CalcTextSizeA(size, FLT_MAX, 0.f, t.c_str()).x; };
    if (width(text) <= maxW) return text;
    while (!text.empty()) {
        do text.pop_back();
        while (!text.empty() && (static_cast<unsigned char>(text.back()) & 0xC0) == 0x80);
        if (width(text + "…") <= maxW) break;
    }
    return text + "…";
}

void smoothScroll() {
    ImGuiID id = ImGui::GetCurrentWindow()->ID;
    auto* store = ImGui::GetStateStorage();
    float& target = *store->GetFloatRef(id ^ 0x5CA1, 0.f);
    float& last = *store->GetFloatRef(id ^ 0x5CA2, 0.f);
    float cur = ImGui::GetScrollY();
    float maxY = ImGui::GetScrollMaxY();
    auto& io = ImGui::GetIO();

    if (std::fabs(cur - last) > 2.f) target = cur;
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && io.MouseWheel != 0.f)
        target -= io.MouseWheel * 84.f * ui::scale();
    target = std::clamp(target, 0.f, maxY);

    float next = draw::approach(cur, target, 13.f * theme::current().animSpeed);
    ImGui::SetScrollY(next);
    last = next;
}

bool statePill(const char* id, ImVec2 min, ImVec2 max, bool on, bool locked) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    ImGui::SetCursorScreenPos(min);
    bool clicked = ImGui::InvisibleButton(id, max - min) && !locked;
    bool hovered = ImGui::IsItemHovered() && !locked;
    float r = (max.y - min.y) * 0.5f;
    const char* label = locked ? i18n::tr("Unavailable") : i18n::tr(on ? "Enabled" : "Disabled");
    if (on && !locked) {
        draw::gradientRect(dl, min, max, theme::col(theme::mix(t.accent, t.accent2, hovered ? 0.35f : 0.f)), theme::col(t.accent2), r);
    } else {
        dl->AddRectFilled(min, max, theme::col(locked ? t.surface : theme::mix(t.surfaceHover, t.text, hovered ? 0.08f : 0.f), locked ? 0.6f : 1.f), r);
    }
    ImVec2 ts = ImGui::CalcTextSize(label);
    ImVec4 tc = on && !locked ? t.bg : (locked ? t.off : t.textDim);
    dl->AddText(fonts::bold(), 14.f * s, {(min.x + max.x) * 0.5f - ts.x * 0.5f, (min.y + max.y) * 0.5f - ts.y * 0.5f}, theme::col(tc), label);
    return clicked;
}

void drawLogo(ImDrawList* dl, ImVec2 p) {
    auto& t = theme::current();
    float s = ui::scale();
    float bob = std::sin((float)ui::time() * 2.2f) * 1.5f * s;
    draw::glow(dl, p + ImVec2(4 * s, 4 * s + bob), p + ImVec2(28 * s, 28 * s + bob), 12 * s, theme::col(t.accent, 0.6f), 10 * s);
    draw::heart(dl, p + ImVec2(16 * s, 17 * s + bob), 30 * s, theme::col(t.accent));
    draw::heart(dl, p + ImVec2(12 * s, 13 * s + bob), 8 * s, theme::col(t.accent2, 0.9f));
    dl->AddText(fonts::bold(), 26 * s, p + ImVec2(38 * s, 1 * s), theme::col(t.text), build::name);
    if (t.sparkles) {
        float tw = 0.5f + 0.5f * std::sin((float)ui::time() * 3.1f);
        draw::sparkle(dl, p + ImVec2(34 * s, 0), 4 * s * tw + 1 * s, theme::col(t.accent2, tw));
    }
}

void drawServerChip(ImVec2 rowMin, ImVec2 rowMax, float right) {
    auto& t = theme::current();
    float s = ui::scale();
    auto info = rules::status();
    std::string label = info.server.empty() ? std::string(i18n::tr("No server")) : info.server;
    if (info.blocked) label += "  ·  " + i18n::fmt("{} blocked", info.blocked);
    if (info.warned) label += "  ·  " + i18n::fmt("{} notices", info.warned);

    ImVec2 ts = ImGui::CalcTextSize(label.c_str());
    ImVec2 size{ts.x + 30 * s, 34 * s};
    ImVec2 p{right - size.x, rowMin.y + (rowMax.y - rowMin.y - size.y) * 0.5f};
    ImGui::SetCursorScreenPos(p);
    ImGui::InvisibleButton("server", size);
    auto* dl = ImGui::GetWindowDrawList();
    ImVec4 dot = info.server.empty() ? t.off : (info.blocked ? t.warn : t.ok);
    dl->AddRectFilled(p, p + size, theme::col(t.surface), size.y * 0.5f);
    dl->AddCircleFilled({p.x + 14 * s, p.y + size.y * 0.5f}, 4 * s, theme::col(dot));
    dl->AddText({p.x + 24 * s, p.y + (size.y - ts.y) * 0.5f}, theme::col(t.textDim), label.c_str());

    if (ImGui::IsItemHovered() && (!info.notes.empty() || !info.notice.empty())) {
        ImGui::BeginTooltip();
        if (!info.notice.empty()) ImGui::TextColored(t.warn, "%s", info.notice.c_str());
        for (auto& n : info.notes) ImGui::TextUnformatted(n.c_str());
        if (!info.rulesUrl.empty()) ImGui::TextColored(t.textDim, i18n::tr("Rules: %s"), info.rulesUrl.c_str());
        ImGui::EndTooltip();
    }
}

void star(ImDrawList* dl, ImVec2 c, float r, ImU32 col, bool filled) {
    ImVec2 pts[10];
    for (int i = 0; i < 10; i++) {
        float a = -1.5708f + i * 0.62832f;
        float rr = i % 2 ? r * 0.45f : r;
        pts[i] = {c.x + std::cos(a) * rr, c.y + std::sin(a) * rr};
    }
    if (filled) dl->AddConvexPolyFilled(pts, 10, col);
    else dl->AddPolyline(pts, 10, col, ImDrawFlags_Closed, 1.4f);
}

}
