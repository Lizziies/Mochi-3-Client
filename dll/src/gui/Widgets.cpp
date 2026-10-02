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
#include <cmath>

namespace widgets {

static ImGuiID capturing = 0;
static bool capturedThisFrame = false;

void drawSwitch(ImDrawList* dl, ImVec2 p, float a, float alpha) {
    auto& t = theme::current();
    float s = ui::scale();
    ImVec2 size = switchSize();
    ImVec4 track = theme::mix(t.off, theme::mix(t.bg, t.accent, 0.35f), a);
    dl->AddRectFilled(p, p + size, theme::col(track, (0.55f + 0.45f * a) * alpha), size.y * 0.5f);
    float r = 3.6f * s;
    float x = p.x + size.y * 0.5f + (size.x - size.y) * draw::easeOutCubic(a);
    ImVec4 knob = theme::mix(ImVec4(1, 1, 1, 1), t.accent2, a);
    dl->AddCircleFilled({x, p.y + size.y * 0.5f}, r, theme::col(knob, alpha), 16);
}

ImVec2 switchSize() {
    float s = ui::scale();
    return {24 * s, 13 * s};
}

bool toggle(const char* id, bool& value, bool enabled) {
    auto& t = theme::current();
    ImVec2 size = switchSize();
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::PushID(id);
    bool clicked = ImGui::InvisibleButton("##toggle", size) && enabled;
    ImGuiID key = ImGui::GetID("anim");
    ImGui::PopID();
    if (clicked) {
        value = !value;
        config::markDirty();
    }
    float& a = *ImGui::GetStateStorage()->GetFloatRef(key, value ? 1.f : 0.f);
    a = draw::approach(a, value ? 1.f : 0.f, 18.f * t.animSpeed);
    drawSwitch(ImGui::GetWindowDrawList(), p, a, enabled ? 1.f : 0.4f);
    return clicked;
}

static bool pill(const char* id, const char* label, bool primary, ImVec2 at, float h, float& width) {
    auto& t = theme::current();
    float s = ui::scale();
    ImVec2 ts = fonts::regular()->CalcTextSizeA(12 * s, FLT_MAX, 0.f, label);
    width = ts.x + 18 * s;
    ImGui::SetCursorScreenPos(at);
    bool clicked = ImGui::InvisibleButton(id, {width, h});
    bool hov = ImGui::IsItemHovered();
    auto* dl = ImGui::GetWindowDrawList();
    ImVec4 bg = primary ? (hov ? theme::mix(t.accent, t.accent2, 0.3f) : t.accent) : (hov ? t.surfaceHover : t.surface);
    dl->AddRectFilled(at, at + ImVec2(width, h), theme::col(bg), 4 * s);
    dl->AddText(fonts::regular(), 12 * s, at + ImVec2(9 * s, (h - ts.y) * 0.5f), theme::col(primary ? ImVec4(1, 1, 1, 1) : t.text), label);
    return clicked;
}

bool button(const char* id, ImVec2 size, bool primary) {
    auto& t = theme::current();
    float s = ui::scale();
    const char* label = i18n::tr(id);
    const char* end = ImGui::FindRenderedTextEnd(label);
    ImVec2 ts = fonts::regular()->CalcTextSizeA(13.5f * s, FLT_MAX, 0.f, label, end);
    if (size.x <= 0) size.x = ts.x + 24 * s;
    if (size.y <= 0) size.y = 28 * s;
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::InvisibleButton(id, size);
    bool hovered = ImGui::IsItemHovered();
    bool held = ImGui::IsItemActive();
    auto* dl = ImGui::GetWindowDrawList();
    float r = 5 * s;
    if (primary) dl->AddRectFilled(p, p + size, theme::col(held ? t.accent2 : hovered ? theme::mix(t.accent, t.accent2, 0.3f) : t.accent), r);
    else dl->AddRectFilled(p, p + size, theme::col(t.surface, hovered ? 1.f : 0.7f), r);
    dl->AddText(fonts::regular(), 13.5f * s, p + (size - ts) * 0.5f, theme::col(primary ? ImVec4(1, 1, 1, 1) : t.text), label, end);
    return clicked;
}

void sectionTitle(const char* text) {
    auto& t = theme::current();
    float s = ui::scale();
    ImGui::Dummy({0, 6 * s});
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddText(fonts::bold(), 13.5f * s, p + ImVec2(2 * s, 0), theme::col(t.text), i18n::tr(text));
    ImGui::Dummy({0, 20 * s});
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

    std::string label = active ? "..." : (vk ? keyName(vk) : std::string(i18n::tr("none")));
    ImVec2 ts = fonts::regular()->CalcTextSizeA(12 * s, FLT_MAX, 0.f, label.c_str());
    ImVec2 size{std::max(40 * s, ts.x + 18 * s), 19 * s};
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::PushID(id);
    bool clicked = ImGui::InvisibleButton("##key", size);
    bool rclick = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    bool hov = ImGui::IsItemHovered();
    ImGui::PopID();

    auto* dl = ImGui::GetWindowDrawList();
    if (active) dl->AddRectFilled(p, p + size, theme::col(t.accent, 0.25f), 4 * s);
    dl->AddRect(p, p + size, theme::col(active ? t.accent2 : (hov ? t.text : t.textDim), 0.9f), 4 * s, 0, 1.2f * s);
    dl->AddText(fonts::regular(), 12 * s, p + (size - ts) * 0.5f, theme::col(active ? t.accent2 : t.text), label.c_str());

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

static std::string hexOf(const ImVec4& c) {
    auto b = [](float x) { return (int)std::lround(std::clamp(x, 0.f, 1.f) * 255.f); };
    char buf[16];
    if (b(c.w) < 255) snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", b(c.x), b(c.y), b(c.z), b(c.w));
    else snprintf(buf, sizeof(buf), "#%02X%02X%02X", b(c.x), b(c.y), b(c.z));
    return buf;
}

static void rowBack(ImDrawList* dl, ImVec2 p, ImVec2 size, bool hov) {
    auto& t = theme::current();
    dl->AddRectFilled(p, p + size, theme::col(t.surface, hov ? 0.85f : 0.6f), 6 * ui::scale());
}

static void rowLabel(ImDrawList* dl, ImVec2 p, float h, const char* label, ImU32 col) {
    float s = ui::scale();
    ImVec2 ts = fonts::regular()->CalcTextSizeA(14.5f * s, FLT_MAX, 0.f, label);
    dl->AddText(fonts::regular(), 14.5f * s, {p.x + 10 * s, p.y + (h - ts.y) * 0.5f}, col, label);
}

// the whole row is the slider, filled from the left like a progress bar
static bool sliderRow(ImVec2 p, ImVec2 size, const char* label, float& v, float lo, float hi, const char* fmt, bool isInt) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    ImGui::SetCursorScreenPos(p);
    ImGui::InvisibleButton("slider", size);
    bool active = ImGui::IsItemActive();
    bool hov = ImGui::IsItemHovered();
    bool changed = false;
    if (active) {
        float k = std::clamp((ImGui::GetIO().MousePos.x - p.x) / std::max(1.f, size.x), 0.f, 1.f);
        float nv = lo + k * (hi - lo);
        if (isInt) nv = std::round(nv);
        if (nv != v) {
            v = nv;
            changed = true;
        }
    }
    float frac = hi > lo ? std::clamp((v - lo) / (hi - lo), 0.f, 1.f) : 0.f;
    rowBack(dl, p, size, hov);
    float& shown = *ImGui::GetStateStorage()->GetFloatRef(ImGui::GetID("fill"), frac);
    shown = active ? frac : draw::approach(shown, frac, 22.f * t.animSpeed);
    if (shown > 0.001f) dl->AddRectFilled(p, {p.x + std::max(12 * s, size.x * shown), p.y + size.y}, theme::col(t.accent, active ? 1.f : 0.9f), 6 * s);
    rowLabel(dl, p, size.y, label, theme::col(t.text));
    char buf[32];
    if (isInt) snprintf(buf, sizeof(buf), "%d", (int)std::lround(v));
    else snprintf(buf, sizeof(buf), fmt, v);
    ImVec2 vs = fonts::regular()->CalcTextSizeA(14 * s, FLT_MAX, 0.f, buf);
    dl->AddText(fonts::regular(), 14 * s, {p.x + size.x - vs.x - 10 * s, p.y + (size.y - vs.y) * 0.5f}, theme::col(t.text), buf);
    return changed;
}

bool setting(Setting& st) {
    if (!st.shown()) return false;
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    float h = 31 * s;
    ImVec2 size{w, h};
    const char* label = i18n::tr(st.label.c_str());
    bool changed = false;
    float right = p.x + w - 10 * s;
    float cy = p.y + h * 0.5f;

    ImGui::PushID(st.id.c_str());
    switch (st.type) {
    case SettingType::Bool: {
        ImGui::SetCursorScreenPos(p);
        bool clicked = ImGui::InvisibleButton("row", size);
        rowBack(dl, p, size, ImGui::IsItemHovered());
        rowLabel(dl, p, h, label, theme::col(t.text));
        if (clicked) {
            st.b = !st.b;
            changed = true;
        }
        float& a = *ImGui::GetStateStorage()->GetFloatRef(ImGui::GetID("anim"), st.b ? 1.f : 0.f);
        a = draw::approach(a, st.b ? 1.f : 0.f, 18.f * t.animSpeed);
        ImVec2 sw = switchSize();
        drawSwitch(dl, {right - sw.x, cy - sw.y * 0.5f}, a, 1.f);
        break;
    }
    case SettingType::Float:
        changed = sliderRow(p, size, label, st.f, st.fmin, st.fmax, st.format, false);
        break;
    case SettingType::Int: {
        float v = (float)st.i;
        changed = sliderRow(p, size, label, v, (float)st.imin, (float)st.imax, "%d", true);
        if (changed) st.i = (int)std::lround(v);
        break;
    }
    case SettingType::Color: {
        rowBack(dl, p, size, ImGui::IsMouseHoveringRect(p, p + size));
        rowLabel(dl, p, h, label, theme::col(t.text));
        float sw = 26 * s, sh = 15 * s;
        ImVec2 swp{right - sw, cy - sh * 0.5f};
        dl->AddRectFilled(swp, swp + ImVec2(sw, sh), theme::col(st.color), 4 * s);
        dl->AddRect(swp, swp + ImVec2(sw, sh), theme::col(t.text, 0.25f), 4 * s);
        float pw = fonts::regular()->CalcTextSizeA(12 * s, FLT_MAX, 0.f, i18n::tr("Change Color")).x + 18 * s;
        ImGui::SetCursorScreenPos(swp);
        bool swClicked = ImGui::InvisibleButton("swatch", {sw, sh});
        bool pillClicked = pill("change", i18n::tr("Change Color"), true, {swp.x - pw - 8 * s, cy - 9.5f * s}, 19 * s, pw);
        if (swClicked || pillClicked) ImGui::OpenPopup("picker");
        if (ImGui::BeginPopup("picker")) {
            float c[4] = {st.color.x, st.color.y, st.color.z, st.color.w};
            if (ImGui::ColorPicker4("##pick", c, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview)) {
                st.color = {c[0], c[1], c[2], c[3]};
                changed = true;
            }
            ImGui::TextDisabled("%s", hexOf(st.color).c_str());
            ImGui::EndPopup();
        }
        break;
    }
    case SettingType::Choice: {
        ImGui::SetCursorScreenPos(p);
        bool clicked = ImGui::InvisibleButton("row", size);
        bool hov = ImGui::IsItemHovered();
        rowBack(dl, p, size, hov);
        rowLabel(dl, p, h, label, theme::col(t.text));
        const char* value = st.choices.empty() ? "" : i18n::tr(st.choices[std::clamp(st.i, 0, (int)st.choices.size() - 1)].c_str());
        ImVec2 vs = fonts::regular()->CalcTextSizeA(12.5f * s, FLT_MAX, 0.f, value);
        ImVec2 bmin{right - vs.x - 28 * s, cy - 10 * s}, bmax{right, cy + 10 * s};
        dl->AddRect(bmin, bmax, theme::col(hov ? t.text : t.textDim, 0.8f), 4 * s, 0, 1.2f * s);
        dl->AddText(fonts::regular(), 12.5f * s, {bmin.x + 9 * s, cy - vs.y * 0.5f}, theme::col(t.text), value);
        ImVec2 ac{bmax.x - 9 * s, cy};
        dl->AddTriangleFilled(ac + ImVec2(-3.5f * s, -1.5f * s), ac + ImVec2(3.5f * s, -1.5f * s), ac + ImVec2(0, 2.5f * s), theme::col(t.textDim));
        if (clicked) ImGui::OpenPopup("choices");
        if (ImGui::BeginPopup("choices")) {
            for (int i = 0; i < (int)st.choices.size(); i++)
                if (ImGui::Selectable(i18n::tr(st.choices[i].c_str()), i == st.i)) {
                    st.i = i;
                    changed = true;
                }
            ImGui::EndPopup();
        }
        break;
    }
    case SettingType::Key: {
        rowBack(dl, p, size, ImGui::IsMouseHoveringRect(p, p + size));
        rowLabel(dl, p, h, label, theme::col(t.text));
        std::string name = st.i ? keyName(st.i) : std::string(i18n::tr("none"));
        float kw = std::max(40 * s, fonts::regular()->CalcTextSizeA(12 * s, FLT_MAX, 0.f, name.c_str()).x + 18 * s);
        ImGui::SetCursorScreenPos({right - kw, cy - 9.5f * s});
        changed = keyCapture("key", st.i);
        float uw = 0.f;
        float unbindW = fonts::regular()->CalcTextSizeA(12 * s, FLT_MAX, 0.f, i18n::tr("Unbind")).x + 18 * s;
        if (pill("unbind", i18n::tr("Unbind"), true, {right - kw - 8 * s - unbindW, cy - 9.5f * s}, 19 * s, uw)) {
            st.i = 0;
            changed = true;
        }
        break;
    }
    case SettingType::Text: {
        rowBack(dl, p, size, false);
        rowLabel(dl, p, h, label, theme::col(t.text));
        char buf[256];
        snprintf(buf, sizeof(buf), "%s", st.text.c_str());
        float iw = std::min(w * 0.5f, 240 * s);
        ImGui::SetCursorScreenPos({right - iw, cy - 11 * s});
        ImGui::SetNextItemWidth(iw);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, theme::col(t.bg, 0.7f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4 * s);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {8 * s, 3 * s});
        if (ImGui::InputText("##v", buf, sizeof(buf))) {
            st.text = buf;
            changed = true;
        }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
        break;
    }
    }
    ImGui::PopID();
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy({w, h});
    if (changed) config::markDirty();
    return changed;
}

bool row(const char* label, float& v, float lo, float hi, const char* fmt) {
    Setting s{label, label, SettingType::Float};
    s.f = v;
    s.fmin = lo;
    s.fmax = hi;
    s.format = fmt;
    if (!setting(s)) return false;
    v = s.f;
    return true;
}

bool row(const char* label, bool& v) {
    Setting s{label, label, SettingType::Bool};
    s.b = v;
    if (!setting(s)) return false;
    v = s.b;
    return true;
}

bool row(const char* label, ImVec4& c) {
    Setting s{label, label, SettingType::Color};
    s.color = c;
    if (!setting(s)) return false;
    c = s.color;
    return true;
}

}
