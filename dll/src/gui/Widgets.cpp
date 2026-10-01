#include "I18n.hpp"
#include "Widgets.hpp"
#include "Theme.hpp"
#include "core/Config.hpp"
#include "render/Draw.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"

#include <imgui_internal.h>
#include <windows.h>

#include <algorithm>

namespace widgets {

static ImGuiID capturing = 0;
static bool capturedThisFrame = false;

bool toggle(const char* id, bool& value, bool enabled) {
    auto& t = theme::current();
    float s = ui::scale();
    ImVec2 size{40 * s, 22 * s};
    ImVec2 p = ImGui::GetCursorScreenPos();

    ImGui::PushID(id);
    bool clicked = ImGui::InvisibleButton("##toggle", size) && enabled;
    bool hovered = ImGui::IsItemHovered();
    ImGuiID key = ImGui::GetID("anim");
    ImGui::PopID();

    if (clicked) {
        value = !value;
        config::markDirty();
    }

    float& a = *ImGui::GetStateStorage()->GetFloatRef(key, value ? 1.f : 0.f);
    a = draw::approach(a, value ? 1.f : 0.f, 18.f * t.animSpeed);

    auto* dl = ImGui::GetWindowDrawList();
    float alpha = enabled ? 1.f : 0.4f;
    ImVec4 track = theme::mix(t.off, t.accent, a);
    if (hovered && enabled) track = theme::mix(track, t.accent2, 0.15f);
    if (t.gradient && a > 0.01f)
        draw::gradientRect(dl, p, p + size, theme::col(theme::mix(t.off, t.accent, a), alpha),
                           theme::col(theme::mix(t.off, t.accent2, a), alpha), size.y * 0.5f);
    else
        draw::pill(dl, p, p + size, theme::col(track, alpha));

    float r = size.y * 0.5f - 3 * s;
    float x = p.x + size.y * 0.5f + (size.x - size.y) * draw::easeOutCubic(a);
    dl->AddCircleFilled({x, p.y + size.y * 0.5f + 1 * s}, r, IM_COL32(0, 0, 0, int(40 * alpha)));
    dl->AddCircleFilled({x, p.y + size.y * 0.5f}, r, IM_COL32(255, 255, 255, int(255 * alpha)));
    if (a > 0.5f && t.hearts)
        draw::heart(dl, {x, p.y + size.y * 0.5f + 0.5f * s}, r * 1.1f, theme::col(t.accent, (a - 0.5f) * 2 * alpha));
    return clicked;
}

bool button(const char* id, ImVec2 size, bool primary) {
    auto& t = theme::current();
    float s = ui::scale();
    const char* label = i18n::tr(id);
    ImVec2 ts = ImGui::CalcTextSize(label);
    if (size.x <= 0) size.x = ts.x + 28 * s;
    if (size.y <= 0) size.y = ts.y + 14 * s;
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::InvisibleButton(id, size);
    bool hovered = ImGui::IsItemHovered();
    bool held = ImGui::IsItemActive();

    auto* dl = ImGui::GetWindowDrawList();
    float r = t.rounding * 0.6f * s;
    if (primary) {
        ImVec4 a = held ? t.accent2 : t.accent;
        if (t.gradient)
            draw::gradientRect(dl, p, p + size, theme::col(a), theme::col(t.accent2), r);
        else
            dl->AddRectFilled(p, p + size, theme::col(a), r);
        if (hovered) draw::glow(dl, p, p + size, r, theme::col(t.accent, 0.8f), 8 * s);
    } else {
        dl->AddRectFilled(p, p + size, theme::col(hovered ? t.surfaceHover : t.surface), r);
    }
    ImVec4 tc = primary ? ImVec4(1, 1, 1, 1) : t.text;
    const char* end = ImGui::FindRenderedTextEnd(label);
    ImVec2 lts = ImGui::CalcTextSize(label, end);
    dl->AddText(p + (size - lts) * 0.5f, theme::col(tc), label, end);
    return clicked;
}

void sectionTitle(const char* text) {
    auto& t = theme::current();
    ImGui::Dummy({0, 4 * ui::scale()});
    ImGui::PushFont(fonts::bold(), 0.f);
    ImGui::TextColored(t.accent, "%s", i18n::tr(text));
    ImGui::PopFont();
}

void hint(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, theme::current().textDim);
    ImGui::TextWrapped("%s", i18n::tr(text));
    ImGui::PopStyleColor();
}

std::string keyName(int vk) {
    switch (vk) {
    case 0: return i18n::tr("None");
    case VK_LBUTTON: return i18n::tr("Mouse left");
    case VK_RBUTTON: return i18n::tr("Mouse right");
    case VK_MBUTTON: return i18n::tr("Mouse wheel");
    case VK_XBUTTON1: return i18n::tr("Mouse 4");
    case VK_XBUTTON2: return i18n::tr("Mouse 5");
    case VK_RSHIFT: return i18n::tr("Right Shift");
    case VK_LSHIFT: return i18n::tr("Left Shift");
    case VK_RCONTROL: return i18n::tr("Right Ctrl");
    case VK_LCONTROL: return i18n::tr("Left Ctrl");
    case VK_LMENU: return i18n::tr("Alt");
    case VK_RMENU: return i18n::tr("Alt Gr");
    case VK_INSERT: return i18n::tr("Insert");
    case VK_DELETE: return i18n::tr("Delete");
    case VK_HOME: return i18n::tr("Home");
    case VK_END: return i18n::tr("End");
    case VK_PRIOR: return i18n::tr("Page up");
    case VK_NEXT: return i18n::tr("Page down");
    case VK_TAB: return i18n::tr("Tab");
    case VK_CAPITAL: return i18n::tr("Caps lock");
    case VK_SPACE: return i18n::tr("Space");
    }
    UINT scan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    switch (vk) {
    case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
    case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END:
        scan |= 0x100;
    }
    wchar_t name[64]{};
    if (GetKeyNameTextW(LONG(scan << 16), name, 64) > 0) {
        char out[128]{};
        WideCharToMultiByte(CP_UTF8, 0, name, -1, out, sizeof(out), nullptr, nullptr);
        return out;
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "0x%02X", vk);
    return buf;
}

bool capturingKey() { return capturing != 0; }

bool keyCapture(const char* id, int& vk) {
    auto& t = theme::current();
    float s = ui::scale();
    ImGuiID gid = ImGui::GetID(id);
    bool active = capturing == gid;

    std::string label = active ? i18n::tr("Press a key…") : keyName(vk);
    ImVec2 size{std::max(110 * s, ImGui::CalcTextSize(label.c_str()).x + 24 * s), ImGui::GetFrameHeight()};
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::PushID(id);
    bool clicked = ImGui::InvisibleButton("##key", size);
    bool rclick = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    ImGui::PopID();

    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, p + size, theme::col(active ? theme::mix(t.surfaceHover, t.accent, 0.25f) : t.surface),
                      t.rounding * 0.5f * s);
    if (active) dl->AddRect(p, p + size, theme::col(t.accent), t.rounding * 0.5f * s, 0, 1.5f * s);
    ImVec2 ts = ImGui::CalcTextSize(label.c_str());
    dl->AddText(p + (size - ts) * 0.5f, theme::col(active ? t.accent : t.text), label.c_str());

    bool changed = false;
    if (clicked && !active) {
        capturing = gid;
        capturedThisFrame = true;
    } else if (rclick) {
        vk = 0;
        capturing = 0;
        changed = true;
    } else if (active && !capturedThisFrame) {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            capturing = 0;
        } else {
            for (int k = 1; k < 255; k++) {
                if (k == VK_LBUTTON) continue;
                if (k == VK_SHIFT || k == VK_CONTROL || k == VK_MENU) continue;
                bool pressed = (GetAsyncKeyState(k) & 0x8000) != 0;
                if (pressed) {
                    vk = k;
                    capturing = 0;
                    changed = true;
                    break;
                }
            }
        }
    }
    capturedThisFrame = false;
    if (changed) config::markDirty();
    return changed;
}

static bool colorEdit(Setting& s) {
    float col[4] = {s.color.x, s.color.y, s.color.z, s.color.w};
    bool changed = ImGui::ColorEdit4(("##" + s.id).c_str(), col,
                                     ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar |
                                         ImGuiColorEditFlags_AlphaPreviewHalf);
    if (changed) s.color = {col[0], col[1], col[2], col[3]};
    return changed;
}

bool setting(Setting& s) {
    if (!s.shown()) return false;
    float sc = ui::scale();
    float labelW = ImGui::GetContentRegionAvail().x * 0.45f;
    bool changed = false;

    ImGui::PushID(s.id.c_str());
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(i18n::tr(s.label.c_str()));
    ImGui::SameLine(labelW);
    ImGui::SetNextItemWidth(-1);

    switch (s.type) {
    case SettingType::Bool: {
        float w = 40 * sc;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - w);
        changed = toggle("v", s.b);
        break;
    }
    case SettingType::Float:
        changed = ImGui::SliderFloat("##v", &s.f, s.fmin, s.fmax, s.format);
        break;
    case SettingType::Int:
        changed = ImGui::SliderInt("##v", &s.i, s.imin, s.imax);
        break;
    case SettingType::Color: {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight());
        changed = colorEdit(s);
        break;
    }
    case SettingType::Choice: {
        const char* preview = s.choices.empty() ? "" : i18n::tr(s.choices[std::clamp(s.i, 0, (int)s.choices.size() - 1)].c_str());
        if (ImGui::BeginCombo("##v", preview)) {
            for (int i = 0; i < (int)s.choices.size(); i++) {
                bool sel = i == s.i;
                if (ImGui::Selectable(i18n::tr(s.choices[i].c_str()), sel)) {
                    s.i = i;
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
        break;
    }
    case SettingType::Key: {
        float w = std::max(110 * sc, ImGui::CalcTextSize(keyName(s.i).c_str()).x + 24 * sc);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - w);
        changed = keyCapture("key", s.i);
        break;
    }
    case SettingType::Text: {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s", s.text.c_str());
        if (ImGui::InputText("##v", buf, sizeof(buf))) {
            s.text = buf;
            changed = true;
        }
        break;
    }
    }
    ImGui::PopID();
    if (changed) config::markDirty();
    return changed;
}

}
