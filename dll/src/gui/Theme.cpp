#include "Theme.hpp"
#include "core/Config.hpp"
#include "render/Ui.hpp"

#include <algorithm>
#include <cstring>

using nlohmann::json;

namespace theme {

static ImVec4 hex(unsigned rgb, float a = 1.f) {
    return {((rgb >> 16) & 0xFF) / 255.f, ((rgb >> 8) & 0xFF) / 255.f, (rgb & 0xFF) / 255.f, a};
}

static std::vector<Theme> builtins = {
    {"Carbon", hex(0x101114), hex(0x191A1F), hex(0x24262D), hex(0x4C8DFF), hex(0x8DB5FF), hex(0xF1F2F5),
     hex(0x8A8F9C), hex(0x3DDC84), hex(0xFFB547), hex(0x3A3D46), 5.f, 0.94f, 1.f, false, false, false, hex(0x2C2F38)},
    {"Graphite", hex(0x121214), hex(0x1D1D21), hex(0x2A2A30), hex(0x8FA6FF), hex(0xC3CEFF), hex(0xEEEEF2),
     hex(0x9D9DAA), hex(0x7BD6A0), hex(0xF2C46B), hex(0x4B4B55), 12.f, 0.96f, 1.f, false, false, false},
    {"Bubblegum", hex(0x1A0F1E), hex(0x2A1730), hex(0x36203D), hex(0xFF7EB6), hex(0xFFB3D1), hex(0xFFF1F7),
     hex(0xC9A9BB), hex(0x8BE9B0), hex(0xFFD27E), hex(0x6B5570)},
    {"Sakura", hex(0xFFF5F8), hex(0xFFE4EC), hex(0xFFD6E3), hex(0xF06292), hex(0xF8A5C2), hex(0x4A2B38),
     hex(0x8E6A79), hex(0x4CAF7A), hex(0xE0A030), hex(0xC9B2BC)},
    {"Lavender", hex(0x17132A), hex(0x241E3D), hex(0x2F2850), hex(0xB69CFF), hex(0xE2D6FF), hex(0xF4F0FF),
     hex(0xA99FC9), hex(0x8BE9B0), hex(0xFFD27E), hex(0x5D5578)},
    {"Strawberry Milk", hex(0xFFF8F3), hex(0xFDEBE4), hex(0xF9DCD3), hex(0xFF8FA3), hex(0xFFC2CC), hex(0x5A3A3F),
     hex(0x9A7A7F), hex(0x5DBB8A), hex(0xE8A33D), hex(0xD4BFC2)},
    {"Midnight Pink", hex(0x0B0710), hex(0x161019), hex(0x221829), hex(0xFF3EA5), hex(0xFF8AC8), hex(0xFFE8F4),
     hex(0xB08AA0), hex(0x6CF0A8), hex(0xFFC857), hex(0x4A3A52)},
};

static Theme active = builtins[0];

Theme& current() { return active; }

ImVec4 border() { return active.border.w > 0.f ? active.border : active.surfaceHover; }
const std::vector<Theme>& presets() { return builtins; }

void use(const Theme& t) {
    active = t;
    applyStyle();
    config::markDirty();
}

static float fadeLevel = 1.f;

void setFade(float f) { fadeLevel = f; }
float fade() { return fadeLevel; }

ImU32 col(const ImVec4& c, float alpha) {
    return ImGui::ColorConvertFloat4ToU32({c.x, c.y, c.z, c.w * alpha * fadeLevel});
}

ImVec4 mix(const ImVec4& a, const ImVec4& b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}

void applyStyle() {
    if (!ImGui::GetCurrentContext()) return;
    auto& s = ImGui::GetStyle();
    s = ImGuiStyle();
    auto& t = active;
    s.WindowRounding = t.rounding;
    s.ChildRounding = t.rounding * 0.75f;
    s.FrameRounding = t.rounding * 0.6f;
    s.PopupRounding = t.rounding * 0.6f;
    s.GrabRounding = 99.f;
    s.ScrollbarRounding = 99.f;
    s.TabRounding = t.rounding * 0.6f;
    s.WindowBorderSize = 0.f;
    s.ChildBorderSize = 0.f;
    s.PopupBorderSize = 0.f;
    s.FrameBorderSize = 0.f;
    s.WindowPadding = {16, 16};
    s.FramePadding = {10, 6};
    s.ItemSpacing = {10, 8};
    s.ScrollbarSize = 8.f;
    s.GrabMinSize = 12.f;

    auto* c = s.Colors;
    ImVec4 bg = t.bg;
    bg.w = t.opacity;
    c[ImGuiCol_WindowBg] = bg;
    c[ImGuiCol_ChildBg] = {0, 0, 0, 0};
    c[ImGuiCol_PopupBg] = t.surface;
    c[ImGuiCol_Text] = t.text;
    c[ImGuiCol_TextDisabled] = t.textDim;
    c[ImGuiCol_FrameBg] = t.surface;
    c[ImGuiCol_FrameBgHovered] = t.surfaceHover;
    c[ImGuiCol_FrameBgActive] = t.surfaceHover;
    c[ImGuiCol_Button] = t.surface;
    c[ImGuiCol_ButtonHovered] = t.surfaceHover;
    c[ImGuiCol_ButtonActive] = mix(t.surfaceHover, t.accent, 0.3f);
    c[ImGuiCol_Header] = t.surface;
    c[ImGuiCol_HeaderHovered] = t.surfaceHover;
    c[ImGuiCol_HeaderActive] = mix(t.surfaceHover, t.accent, 0.3f);
    c[ImGuiCol_SliderGrab] = t.accent;
    c[ImGuiCol_SliderGrabActive] = t.accent2;
    c[ImGuiCol_CheckMark] = t.accent;
    c[ImGuiCol_ScrollbarBg] = {0, 0, 0, 0};
    c[ImGuiCol_ScrollbarGrab] = t.surfaceHover;
    c[ImGuiCol_ScrollbarGrabHovered] = t.accent;
    c[ImGuiCol_ScrollbarGrabActive] = t.accent2;
    c[ImGuiCol_Separator] = t.surfaceHover;
    c[ImGuiCol_TextSelectedBg] = mix(t.accent, t.bg, 0.5f);
    c[ImGuiCol_Border] = t.surfaceHover;
    c[ImGuiCol_NavCursor] = t.accent;

    s.ScaleAllSizes(ui::scale());
    s.FontScaleMain = ui::scale();
}

static json color(const ImVec4& v) { return json::array({v.x, v.y, v.z, v.w}); }

static void color(const json& j, const char* key, ImVec4& out) {
    if (j.contains(key) && j[key].is_array() && j[key].size() == 4)
        out = {j[key][0], j[key][1], j[key][2], j[key][3]};
}

json save() {
    auto& t = active;
    return {
        {"name", t.name},
        {"bg", color(t.bg)},
        {"surface", color(t.surface)},
        {"surfaceHover", color(t.surfaceHover)},
        {"accent", color(t.accent)},
        {"accent2", color(t.accent2)},
        {"text", color(t.text)},
        {"textDim", color(t.textDim)},
        {"ok", color(t.ok)},
        {"warn", color(t.warn)},
        {"off", color(t.off)},
        {"rounding", t.rounding},
        {"opacity", t.opacity},
        {"animSpeed", t.animSpeed},
        {"gradient", t.gradient},
        {"sparkles", t.sparkles},
        {"hearts", t.hearts},
        {"border", color(t.border)},
    };
}

void load(const json& j) {
    if (!j.is_object()) return;
    // the old default; whoever still has it gets the new default look instead
    if (j.value("name", "") == "Graphite" && j.value("accent", json::array()) == color(builtins[1].accent)) {
        active = builtins[0];
        return;
    }
    Theme t = builtins[0];
    t.name = j.value("name", t.name);
    color(j, "bg", t.bg);
    color(j, "surface", t.surface);
    color(j, "surfaceHover", t.surfaceHover);
    color(j, "accent", t.accent);
    color(j, "accent2", t.accent2);
    color(j, "text", t.text);
    color(j, "textDim", t.textDim);
    color(j, "ok", t.ok);
    color(j, "warn", t.warn);
    color(j, "off", t.off);
    t.rounding = std::clamp(j.value("rounding", t.rounding), 0.f, 30.f);
    t.opacity = std::clamp(j.value("opacity", t.opacity), 0.3f, 1.f);
    t.animSpeed = std::clamp(j.value("animSpeed", t.animSpeed), 0.25f, 3.f);
    t.gradient = j.value("gradient", t.gradient);
    t.sparkles = j.value("sparkles", t.sparkles);
    t.hearts = j.value("hearts", t.hearts);
    color(j, "border", t.border);
    active = t;
}

static const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string exportCode() {
    std::string in = save().dump();
    std::string out = "mochi:";
    int val = 0, bits = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        bits += 8;
        while (bits >= 0) {
            out.push_back(b64[(val >> bits) & 0x3F]);
            bits -= 6;
        }
    }
    if (bits > -6) out.push_back(b64[((val << 8) >> (bits + 8)) & 0x3F]);
    return out;
}

bool importCode(const std::string& code) {
    std::string_view s = code;
    if (s.rfind("mochi:", 0) != 0) return false;
    s.remove_prefix(6);

    std::string out;
    int val = 0, bits = -8;
    for (char c : s) {
        const char* p = std::strchr(b64, c);
        if (!p || !c) break;
        val = (val << 6) + int(p - b64);
        bits += 6;
        if (bits >= 0) {
            out.push_back(char((val >> bits) & 0xFF));
            bits -= 8;
        }
    }
    auto j = json::parse(out, nullptr, false);
    if (j.is_discarded()) return false;
    load(j);
    applyStyle();
    config::markDirty();
    return true;
}

}
