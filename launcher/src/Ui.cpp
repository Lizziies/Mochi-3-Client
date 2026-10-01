#include "Ui.hpp"

#include "Build.hpp"
#include "I18n.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>

namespace ui {

namespace {

using i18n::tr;

ImVec4 hex(unsigned rgb, float a = 1.f) {
    return {((rgb >> 16) & 0xFF) / 255.f, ((rgb >> 8) & 0xFF) / 255.f, (rgb & 0xFF) / 255.f, a};
}

const ImVec4 bg = hex(0x1A0F1E);
const ImVec4 side = hex(0x140B18);
const ImVec4 surface = hex(0x2A1730);
const ImVec4 surfaceHover = hex(0x36203D);
const ImVec4 accent = hex(0xFF7EB6);
const ImVec4 accent2 = hex(0xFFB3D1);
const ImVec4 deep = hex(0x7A2D5C);
const ImVec4 text = hex(0xFFF1F7);
const ImVec4 dim = hex(0xC9A9BB);
const ImVec4 ok = hex(0x8BE9B0);
const ImVec4 warn = hex(0xFFD27E);
const ImVec4 off = hex(0x6B5570);

ImFont* regular = nullptr;
ImFont* bold = nullptr;
float fade = 1.f;

ImU32 col(ImVec4 c, float a = 1.f) {
    c.w *= a * fade;
    return ImGui::ColorConvertFloat4ToU32(c);
}

ImVec4 mix(ImVec4 a, ImVec4 b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}

float approach(float cur, float target, float speed) {
    float v = cur + (target - cur) * (1.f - std::exp(-speed * ImGui::GetIO().DeltaTime));
    return std::fabs(v - target) < 0.001f ? target : v;
}

float& anim(ImGuiID id, float target, float speed = 14.f) {
    float& v = *ImGui::GetStateStorage()->GetFloatRef(id, target);
    v = approach(v, target, speed);
    return v;
}

float easeOut(float t) {
    t = 1.f - std::clamp(t, 0.f, 1.f);
    return 1.f - t * t * t;
}

ImVec2 measure(ImFont* f, float size, const char* s) { return f->CalcTextSizeA(size, FLT_MAX, 0.f, s); }

void label(ImDrawList* dl, ImFont* f, float size, ImVec2 at, ImU32 c, const char* s) {
    dl->AddText(f, size, at, c, s);
}

void centered(ImDrawList* dl, ImFont* f, float size, ImVec2 mid, ImU32 c, const char* s) {
    ImVec2 ts = measure(f, size, s);
    dl->AddText(f, size, {mid.x - ts.x * 0.5f, mid.y - ts.y * 0.5f}, c, s);
}

void gradient(ImDrawList* dl, ImVec2 min, ImVec2 max, ImVec4 a, ImVec4 b, float rounding, bool vertical = false) {
    int start = dl->VtxBuffer.Size;
    dl->AddRectFilled(min, max, IM_COL32_WHITE, rounding);
    float span = std::max(1.f, vertical ? max.y - min.y : max.x - min.x);
    for (int i = start; i < dl->VtxBuffer.Size; i++) {
        auto& v = dl->VtxBuffer[i];
        float t = std::clamp(((vertical ? v.pos.y - min.y : v.pos.x - min.x)) / span, 0.f, 1.f);
        v.col = col(mix(a, b, t));
    }
}

void heart(ImDrawList* dl, ImVec2 c, float size, ImU32 color) {
    constexpr int n = 48;
    ImVec2 pts[n];
    float k = size / 34.f;
    for (int i = 0; i < n; i++) {
        float t = float(i) / n * 6.2831853f;
        float x = 16.f * std::pow(std::sin(t), 3.f);
        float y = 13.f * std::cos(t) - 5.f * std::cos(2.f * t) - 2.f * std::cos(3.f * t) - std::cos(4.f * t);
        pts[i] = {c.x + x * k, c.y - y * k + size * 0.05f};
    }
    ImDrawListFlags saved = dl->Flags;
    dl->Flags &= ~ImDrawListFlags_AntiAliasedFill;
    ImVec2 mid{c.x, c.y + size * 0.05f};
    for (int i = 0; i < n; i++) dl->AddTriangleFilled(mid, pts[i], pts[(i + 1) % n], color);
    dl->Flags = saved;
}

void sparkle(ImDrawList* dl, ImVec2 c, float s, ImU32 color) {
    float w = s * 0.2f;
    ImVec2 pts[8] = {{c.x, c.y - s},     {c.x + w, c.y - w}, {c.x + s, c.y}, {c.x + w, c.y + w},
                     {c.x, c.y + s},     {c.x - w, c.y + w}, {c.x - s, c.y}, {c.x - w, c.y - w}};
    dl->AddConcavePolyFilled(pts, 8, color);
}

void pixelHeart(ImDrawList* dl, ImVec2 pos, float px) {
    static const char* rows[] = {
        "..ee...ee..", ".eHHe.eeee.", "eHHeeeeeeee", "eHeeeeeeeee", "eeeeeeeeeed",
        ".eeeeeeeed.", "..eeeeeed..", "...eeeed...", "....eed....", ".....d.....",
    };
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 11; x++) {
            char c = rows[y][x];
            if (c == '.') continue;
            ImVec4 k = c == 'H' ? accent2 : c == 'd' ? mix(accent, deep, 0.45f) : accent;
            ImVec2 p{pos.x + x * px, pos.y + y * px};
            dl->AddRectFilled(p, {p.x + px, p.y + px}, col(k));
        }
    }
}

void chip(ImDrawList* dl, ImVec2 at, const char* s, ImVec4 c) {
    ImVec2 ts = measure(bold, 13.f, s);
    ImVec2 max{at.x + ts.x + 22.f, at.y + 24.f};
    dl->AddRectFilled(at, max, col(c, 0.16f), 12.f);
    label(dl, bold, 13.f, {at.x + 11.f, at.y + 12.f - ts.y * 0.5f}, col(c), s);
}

float chipWidth(const char* s) { return measure(bold, 13.f, s).x + 22.f; }

bool region(const char* id, ImVec2 min, ImVec2 max, bool& hovered, bool& held) {
    ImGui::SetCursorScreenPos(min);
    bool clicked = ImGui::InvisibleButton(id, {max.x - min.x, max.y - min.y});
    hovered = ImGui::IsItemHovered();
    held = ImGui::IsItemActive();
    return clicked;
}

bool button(const char* id, ImVec2 min, ImVec2 max, const char* s, bool primary, bool enabled = true) {
    bool hovered, held;
    bool clicked = region(id, min, max, hovered, held) && enabled;
    float& h = anim(ImGui::GetItemID(), hovered && enabled ? 1.f : 0.f);
    float lift = h * 2.f - (held ? 2.f : 0.f);
    ImVec2 a{min.x, min.y - lift}, b{max.x, max.y - lift};
    float r = (b.y - a.y) * 0.5f;
    auto* dl = ImGui::GetWindowDrawList();

    if (primary && enabled) {
        dl->AddRectFilled({a.x, a.y + 6.f}, {b.x, b.y + 6.f + h * 3.f}, col(accent, 0.18f + 0.1f * h), r + 4.f);
        gradient(dl, a, b, mix(accent, accent2, h * 0.25f), mix(accent2, accent, 0.15f), r);
        centered(dl, bold, 20.f, {(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f}, col(hex(0x3A1030)), s);
    } else if (primary) {
        dl->AddRectFilled(a, b, col(surfaceHover), r);
        centered(dl, bold, 20.f, {(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f}, col(dim), s);
    } else {
        dl->AddRectFilled(a, b, col(mix(surfaceHover, accent, h * 0.18f)), r);
        centered(dl, bold, 15.f, {(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f}, col(enabled ? text : off), s);
    }
    return clicked;
}

bool toggle(const char* id, ImVec2 pos, bool& value) {
    ImVec2 size{48.f, 26.f};
    bool hovered, held;
    bool clicked = region(id, pos, {pos.x + size.x, pos.y + size.y}, hovered, held);
    if (clicked) value = !value;
    float& t = anim(ImGui::GetItemID(), value ? 1.f : 0.f, 16.f);
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, {pos.x + size.x, pos.y + size.y}, col(mix(off, accent, t)), 13.f);
    float spring = easeOut(t);
    float kx = pos.x + 13.f + (size.x - 26.f) * spring;
    dl->AddCircleFilled({kx, pos.y + 13.f}, 9.5f + (hovered ? 0.8f : 0.f), col(mix(dim, text, t)), 24);
    return clicked;
}

enum class Icon { Play, Layers, Sliders, Info };

void icon(ImDrawList* dl, Icon kind, ImVec2 c, ImU32 color) {
    switch (kind) {
    case Icon::Play:
        dl->AddTriangleFilled({c.x - 5.f, c.y - 8.f}, {c.x - 5.f, c.y + 8.f}, {c.x + 9.f, c.y}, color);
        break;
    case Icon::Layers:
        for (int i = 0; i < 3; i++)
            dl->AddRectFilled({c.x - 9.f, c.y - 9.f + i * 6.5f}, {c.x + 9.f, c.y - 5.f + i * 6.5f}, color, 2.f);
        break;
    case Icon::Sliders:
        for (int i = 0; i < 3; i++) {
            float y = c.y - 7.f + i * 7.f;
            dl->AddLine({c.x - 9.f, y}, {c.x + 9.f, y}, color, 2.f);
            dl->AddCircleFilled({c.x - 4.f + i * 4.f, y}, 3.2f, color, 12);
        }
        break;
    case Icon::Info:
        dl->AddCircle(c, 9.f, color, 24, 2.f);
        dl->AddRectFilled({c.x - 1.f, c.y - 1.f}, {c.x + 1.f, c.y + 5.f}, color);
        dl->AddCircleFilled({c.x, c.y - 4.f}, 1.4f, color, 8);
        break;
    }
}

ImVec2 cardMin(float x, float y) { return {x, y}; }

void card(ImDrawList* dl, ImVec2 min, ImVec2 max, ImVec4 fill = surface) {
    dl->AddRectFilled(min, max, col(fill), 18.f);
}

Page lastPage = Page::Start;
float pageAge = 10.f;

struct Reveal {
    ImDrawList* dl;
    int start;
    float shift;
    float before;

    Reveal(ImDrawList* list, int index) : dl(list), start(list->VtxBuffer.Size), before(fade) {
        float e = easeOut((pageAge - index * 0.07f) / 0.38f);
        shift = (1.f - e) * 18.f;
        fade = before * e;
    }

    ~Reveal() {
        for (int i = start; i < dl->VtxBuffer.Size; i++) dl->VtxBuffer[i].pos.y += shift;
        fade = before;
    }
};

const char* playLabel(Phase p) {
    switch (p) {
    case Phase::Idle: return tr("Play");
    case Phase::Updating: return tr("Updating");
    case Phase::Starting: return tr("Starting Minecraft");
    case Phase::Waiting: return tr("Waiting for the game");
    case Phase::Injecting: return tr("Connecting");
    case Phase::Done: return tr("Running");
    case Phase::Failed: return tr("Try again");
    }
    return "";
}

bool busy(Phase p) {
    return p == Phase::Updating || p == Phase::Starting || p == Phase::Waiting || p == Phase::Injecting;
}

void sidebar(ImDrawList* dl, State& s, Events& ev) {
    dl->AddRectFilled({0, 0}, {232.f, height}, col(side));
    pixelHeart(dl, {28.f, 26.f}, 4.f);
    label(dl, bold, 28.f, {84.f, 28.f}, col(text), "Mochi");

    struct Item { const char* name; Icon icon; Page page; };
    const Item items[] = {
        {tr("Home"), Icon::Play, Page::Start},
        {tr("Versions"), Icon::Layers, Page::Versions},
        {tr("Settings"), Icon::Sliders, Page::Settings},
    };

    float rowH = 48.f, top = 118.f;
    float& marker = *ImGui::GetStateStorage()->GetFloatRef(ImGui::GetID("marker"), top);
    int active = 0;
    for (int i = 0; i < 3; i++)
        if (items[i].page == s.page) active = i;
    marker = approach(marker, top + active * (rowH + 4.f), 14.f);

    dl->AddRectFilled({14.f, marker}, {218.f, marker + rowH}, col(accent, 0.16f), 14.f);
    dl->AddRectFilled({14.f, marker + 12.f}, {18.f, marker + rowH - 12.f}, col(accent), 3.f);

    for (int i = 0; i < 3; i++) {
        float y = top + i * (rowH + 4.f);
        bool hovered, held;
        if (region(items[i].name, {14.f, y}, {218.f, y + rowH}, hovered, held)) s.page = items[i].page;
        float& h = anim(ImGui::GetItemID(), hovered ? 1.f : 0.f);
        bool on = items[i].page == s.page;
        if (!on && h > 0.f) dl->AddRectFilled({14.f, y}, {218.f, y + rowH}, col(accent, 0.07f * h), 14.f);
        ImVec4 c = on ? accent : mix(dim, text, h);
        icon(dl, items[i].icon, {44.f, y + rowH * 0.5f}, col(c));
        label(dl, on ? bold : regular, 17.f, {68.f, y + rowH * 0.5f - 10.f}, col(on ? text : c), items[i].name);
    }

    float by = height - 74.f;
    label(dl, regular, 14.f, {28.f, by + 8.f}, col(dim), (std::string(tr("Version")) + " " + s.clientVersion).c_str());
    if (s.updateAvailable) {
        dl->AddCircleFilled({32.f, by + 38.f}, 4.f, col(warn), 12);
        label(dl, bold, 13.f, {44.f, by + 30.f}, col(warn), tr("Update available"));
    } else {
        dl->AddCircleFilled({32.f, by + 38.f}, 4.f, col(ok), 12);
        label(dl, regular, 13.f, {44.f, by + 30.f}, col(ok), tr("Up to date"));
    }
}

void titlebar(ImDrawList* dl, Events& ev) {
    float x = width - controlsWidth;
    bool hovered, held;
    if (region("min", {x, 0.f}, {x + 48.f, titleHeight}, hovered, held)) ev.minimize = true;
    float& hm = anim(ImGui::GetItemID(), hovered ? 1.f : 0.f);
    dl->AddRectFilled({x + 8.f, 8.f}, {x + 40.f, titleHeight - 8.f}, col(surfaceHover, hm), 10.f);
    dl->AddLine({x + 17.f, titleHeight * 0.5f}, {x + 31.f, titleHeight * 0.5f}, col(mix(dim, text, hm)), 2.f);

    if (region("close", {x + 48.f, 0.f}, {x + 96.f, titleHeight}, hovered, held)) ev.close = true;
    float& hc = anim(ImGui::GetItemID(), hovered ? 1.f : 0.f);
    dl->AddRectFilled({x + 56.f, 8.f}, {x + 88.f, titleHeight - 8.f}, col(mix(surfaceHover, accent, 0.6f), hc), 10.f);
    ImVec2 c{x + 72.f, titleHeight * 0.5f};
    ImU32 xc = col(mix(dim, hex(0x3A1030), hc));
    dl->AddLine({c.x - 6.f, c.y - 6.f}, {c.x + 6.f, c.y + 6.f}, xc, 2.f);
    dl->AddLine({c.x + 6.f, c.y - 6.f}, {c.x - 6.f, c.y + 6.f}, xc, 2.f);
}

void progressBar(ImDrawList* dl, ImVec2 min, ImVec2 max, float value, bool indeterminate) {
    float r = (max.y - min.y) * 0.5f;
    dl->AddRectFilled(min, max, col(hex(0xFFFFFF), 0.18f), r);
    float w = max.x - min.x;
    float t = float(ImGui::GetTime());
    if (indeterminate) {
        float seg = w * 0.32f;
        float x = min.x + (w + seg) * std::fmod(t * 0.55f, 1.f) - seg;
        dl->PushClipRect(min, max, true);
        dl->AddRectFilled({x, min.y}, {x + seg, max.y}, col(text, 0.95f), r);
        dl->PopClipRect();
        return;
    }
    float fill = std::max(r * 2.f, w * std::clamp(value, 0.f, 1.f));
    dl->AddRectFilled(min, {min.x + fill, max.y}, col(text, 0.95f), r);
}

void hero(ImDrawList* dl, State& s, Events& ev) {
    Reveal reveal(dl, 0);
    ImVec2 min{264.f, 62.f}, max{928.f, 292.f};
    gradient(dl, min, max, mix(accent, deep, 0.15f), deep, 24.f);

    dl->PushClipRect(min, max, true);
    heart(dl, {max.x - 120.f, min.y + 130.f}, 260.f, col(hex(0xFFFFFF), 0.10f));
    heart(dl, {max.x - 70.f, min.y + 52.f}, 70.f, col(hex(0xFFFFFF), 0.14f));
    float t = float(ImGui::GetTime());
    sparkle(dl, {max.x - 250.f, min.y + 46.f}, 8.f + 2.f * std::sin(t * 2.f), col(text, 0.9f));
    sparkle(dl, {max.x - 190.f, min.y + 186.f}, 6.f + 1.5f * std::sin(t * 2.6f + 1.f), col(text, 0.75f));
    sparkle(dl, {max.x - 330.f, min.y + 112.f}, 5.f + 1.5f * std::sin(t * 1.8f + 2.f), col(text, 0.6f));
    dl->PopClipRect();

    label(dl, bold, 38.f, {min.x + 36.f, min.y + 30.f}, col(text), tr("Ready to play"));
    std::string sub = "Minecraft Bedrock " + (s.gameVersion.empty() ? std::string(tr("not found")) : s.gameVersion);
    label(dl, regular, 17.f, {min.x + 38.f, min.y + 80.f}, col(text, 0.82f), sub.c_str());

    bool working = busy(s.phase);
    bool playable = !working && s.phase != Phase::Done;
    ImVec2 bmin{min.x + 36.f, min.y + 118.f}, bmax{min.x + 330.f, min.y + 176.f};
    if (button("play", bmin, bmax, playLabel(s.phase), true, playable)) ev.play = true;
    if (playable) heart(dl, {bmax.x - 38.f, bmin.y + 27.f}, 22.f, col(hex(0x3A1030)));

    if (working || s.phase == Phase::Failed || s.phase == Phase::Done) {
        label(dl, regular, 15.f, {min.x + 36.f, max.y - 34.f}, col(text, 0.9f), s.status.c_str());
    }
    if (working) progressBar(dl, {min.x + 36.f, max.y - 16.f}, {max.x - 36.f, max.y - 10.f}, s.progress, s.progress <= 0.f);
}

void infoCard(ImDrawList* dl, State& s, Events& ev) {
    Reveal reveal(dl, 2);
    ImVec2 min{264.f, 312.f}, max{540.f, 576.f};
    card(dl, min, max);
    label(dl, bold, 18.f, {min.x + 22.f, min.y + 18.f}, col(text), "Status");

    float y = min.y + 62.f;
    label(dl, regular, 14.f, {min.x + 22.f, y}, col(dim), "Mochi");
    label(dl, bold, 18.f, {min.x + 22.f, y + 20.f}, col(text), s.clientVersion.c_str());
    if (s.updateAvailable) chip(dl, {max.x - 22.f - chipWidth("Update"), y + 8.f}, "Update", warn);
    else chip(dl, {max.x - 22.f - chipWidth(tr("Current")), y + 8.f}, tr("Current"), ok);

    y += 78.f;
    label(dl, regular, 14.f, {min.x + 22.f, y}, col(dim), "Minecraft");
    label(dl, bold, 18.f, {min.x + 22.f, y + 20.f}, col(text), s.gameVersion.empty() ? tr("not found") : s.gameVersion.c_str());
    const char* tag = s.gameVersion.empty() ? tr("Missing") : s.gameSupported ? tr("Supported") : tr("Untested");
    ImVec4 tc = s.gameVersion.empty() ? off : s.gameSupported ? ok : warn;
    chip(dl, {max.x - 22.f - chipWidth(tag), y + 8.f}, tag, tc);

    if (s.updateAvailable) {
        std::string t = i18n::fmt("Update to {}", s.latestVersion);
        if (button("update", {min.x + 22.f, max.y - 56.f}, {max.x - 22.f, max.y - 18.f}, t.c_str(), false, !busy(s.phase)))
            ev.update = true;
    }
}

void changelogCard(ImDrawList* dl, State& s) {
    Reveal reveal(dl, 3);
    ImVec2 min{556.f, 312.f}, max{928.f, 576.f};
    card(dl, min, max);
    std::string title = i18n::fmt("What's new in {}", s.latestVersion.empty() ? s.clientVersion : s.latestVersion);
    label(dl, bold, 18.f, {min.x + 22.f, min.y + 18.f}, col(text), title.c_str());

    dl->PushClipRect({min.x, min.y + 52.f}, {max.x, max.y - 10.f}, true);
    std::istringstream in(s.changelog.empty() ? std::string(tr("No release notes yet.")) : s.changelog);
    std::string line;
    float y = min.y + 62.f;
    while (std::getline(in, line) && y < max.y - 24.f) {
        if (line.empty()) continue;
        dl->AddCircleFilled({min.x + 28.f, y + 10.f}, 3.f, col(accent), 10);
        label(dl, regular, 15.f, {min.x + 42.f, y}, col(dim), line.c_str());
        y += 30.f;
    }
    dl->PopClipRect();
}

void header(ImDrawList* dl, const char* title, const char* subtitle) {
    label(dl, bold, 30.f, {264.f, 54.f}, col(text), title);
    label(dl, regular, 16.f, {264.f, 94.f}, col(dim), subtitle);
}

void versions(ImDrawList* dl, State& s) {
    {
        Reveal r(dl, 0);
        header(dl, tr("Versions"), tr("Which Minecraft version do you want to play?"));
    }
    float y = 136.f;
    for (size_t i = 0; i < s.versions.size() && y < 450.f; i++, y += 70.f) {
        Reveal r(dl, int(i) + 1);
        auto& v = s.versions[i];
        ImVec2 min{264.f, y}, max{928.f, y + 60.f};
        card(dl, min, max);
        label(dl, bold, 19.f, {min.x + 22.f, min.y + 10.f}, col(text), v.name.c_str());
        float cx = min.x + 22.f;
        if (v.preview) { chip(dl, {cx, min.y + 33.f}, "Preview", accent2); cx += chipWidth("Preview") + 8.f; }
        if (v.installed) { chip(dl, {cx, min.y + 33.f}, tr("Installed"), dim); cx += chipWidth(tr("Installed")) + 8.f; }
        if (v.supported) chip(dl, {cx, min.y + 33.f}, tr("Mochi compatible"), ok);
        if (v.active) {
            chip(dl, {max.x - 22.f - chipWidth(tr("Active")), min.y + 18.f}, tr("Active"), accent);
        } else {
            std::string id = "sw" + std::to_string(i);
            button(id.c_str(), {max.x - 150.f, min.y + 12.f}, {max.x - 18.f, min.y + 48.f}, tr("Switch"), false, false);
        }
    }

    Reveal note(dl, 6);
    ImVec2 min{264.f, 480.f}, max{928.f, 576.f};
    dl->AddRectFilled(min, max, col(warn, 0.08f), 18.f);
    dl->AddRect(min, max, col(warn, 0.35f), 18.f, 0, 1.5f);
    label(dl, bold, 16.f, {min.x + 22.f, min.y + 16.f}, col(warn), tr("Version switching is still in progress"));
    label(dl, regular, 15.f, {min.x + 22.f, min.y + 44.f}, col(dim), tr("Switching will later work through your own Microsoft license. Until then you play with the installed version."));
}

int segment(const char* id, ImVec2 pos, const char* const* names, int count, int current) {
    float w = 92.f, h = 32.f;
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, {pos.x + w * count, pos.y + h}, col(hex(0x1A0F1E)), h * 0.5f);
    float& slot = *ImGui::GetStateStorage()->GetFloatRef(ImGui::GetID(id), float(current));
    slot = approach(slot, float(current), 16.f);
    dl->AddRectFilled({pos.x + 3.f + slot * w, pos.y + 3.f}, {pos.x + w * (slot + 1.f) - 3.f, pos.y + h - 3.f},
                      col(accent), (h - 6.f) * 0.5f);
    int result = current;
    for (int i = 0; i < count; i++) {
        bool hovered, held;
        std::string item = std::string(id) + std::to_string(i);
        if (region(item.c_str(), {pos.x + i * w, pos.y}, {pos.x + (i + 1) * w, pos.y + h}, hovered, held)) result = i;
        float on = std::clamp(1.f - std::fabs(slot - i), 0.f, 1.f);
        centered(dl, bold, 14.f, {pos.x + (i + 0.5f) * w, pos.y + h * 0.5f}, col(mix(hovered ? text : dim, hex(0x3A1030), on)), names[i]);
    }
    return result;
}

bool settingRow(ImDrawList* dl, const char* id, float y, int index, const char* title, const char* desc, bool& value) {
    Reveal r(dl, index);
    ImVec2 min{264.f, y}, max{928.f, y + 56.f};
    card(dl, min, max);
    label(dl, bold, 16.f, {min.x + 22.f, min.y + 9.f}, col(text), title);
    label(dl, regular, 14.f, {min.x + 22.f, min.y + 31.f}, col(dim), desc);
    return toggle(id, {max.x - 22.f - 48.f, min.y + 15.f}, value);
}

void settings(ImDrawList* dl, State& s, Events& ev) {
    {
        Reveal r(dl, 0);
        header(dl, tr("Settings"), tr("How the launcher should behave."));
    }
    bool changed = false;

    {
        Reveal r(dl, 1);
        ImVec2 lmin{264.f, 130.f}, lmax{928.f, 186.f};
        card(dl, lmin, lmax);
        label(dl, bold, 16.f, {lmin.x + 22.f, lmin.y + 9.f}, col(text), tr("Language"));
        label(dl, regular, 14.f, {lmin.x + 22.f, lmin.y + 31.f}, col(dim), tr("Auto follows your Windows language."));
        const char* names[] = {tr("Auto"), "English", "Deutsch"};
        int now = int(i18n::chosen());
        int pick = segment("lang", {lmax.x - 22.f - 276.f, lmin.y + 12.f}, names, 3, now);
        if (pick != now) i18n::choose(i18n::Lang(pick));
    }

    changed |= settingRow(dl, "beta", 194.f, 2, tr("Beta updates"), tr("Get new versions earlier, even if they may still have bugs."), s.settings.beta);
    changed |= settingRow(dl, "auto", 258.f, 3, tr("Connect automatically"), tr("Connects the client as soon as Minecraft has started."), s.settings.autoInject);
    changed |= settingRow(dl, "close", 322.f, 4, tr("Close launcher afterwards"), tr("Closes this window once the client has loaded."), s.settings.closeAfterInject);

    {
        Reveal r(dl, 5);
        ImVec2 min{264.f, 386.f}, max{928.f, 470.f};
        card(dl, min, max);
        label(dl, bold, 16.f, {min.x + 22.f, min.y + 9.f}, col(text), tr("Custom DLL (for developers)"));
        label(dl, regular, 14.f, {min.x + 22.f, min.y + 31.f}, col(dim), tr("Leave empty for the normal version."));
        ImVec2 fmin{min.x + 22.f, min.y + 50.f}, fmax{max.x - 150.f, min.y + 76.f};
        dl->AddRectFilled(fmin, fmax, col(hex(0x1A0F1E)), 10.f);
        ImGui::SetCursorScreenPos({fmin.x + 10.f, fmin.y + 3.f});
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, text);
        ImGui::SetNextItemWidth(fmax.x - fmin.x - 20.f);
        if (ImGui::InputText("##dll", s.dllPath, sizeof(s.dllPath))) {
            s.settings.customDll = s.dllPath;
            changed = true;
        }
        ImGui::PopStyleColor(2);
        if (button("browse", {max.x - 134.f, min.y + 48.f}, {max.x - 22.f, min.y + 78.f}, tr("Browse"), false)) ev.browseDll = true;
    }

    {
        Reveal r(dl, 6);
        ImVec2 min{264.f, 482.f}, max{928.f, 580.f};
        card(dl, min, max);
        pixelHeart(dl, {min.x + 20.f, min.y + 18.f}, 3.f);
        label(dl, bold, 16.f, {min.x + 62.f, min.y + 14.f}, col(text), (std::string("Mochi Launcher ") + build::version).c_str());
        label(dl, regular, 13.f, {min.x + 62.f, min.y + 38.f}, col(dim), tr("Mochi is an independent project and is not affiliated with Mojang or Microsoft."));
        if (button("logs", {min.x + 22.f, max.y - 38.f}, {min.x + 162.f, max.y - 10.f}, tr("Open logs"), false)) ev.openLogs = true;
        if (button("folder", {min.x + 172.f, max.y - 38.f}, {min.x + 312.f, max.y - 10.f}, tr("Open folder"), false)) ev.openFolder = true;
    }
    ev.settingsChanged |= changed;
}

}

void setFonts(ImFont* r, ImFont* b) {
    regular = r;
    bold = b ? b : r;
}

void draw(State& s, Events& ev) {
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({width, height});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg);
    ImGui::Begin("launcher", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
    auto* dl = ImGui::GetWindowDrawList();

    if (s.page != lastPage) {
        lastPage = s.page;
        pageAge = 0.f;
    }
    pageAge += ImGui::GetIO().DeltaTime;

    fade = 1.f;
    sidebar(dl, s, ev);
    titlebar(dl, ev);

    ImGui::SetCursorScreenPos({0, 0});
    switch (s.page) {
    case Page::Start:
        hero(dl, s, ev);
        infoCard(dl, s, ev);
        changelogCard(dl, s);
        break;
    case Page::Versions: versions(dl, s); break;
    case Page::Settings: settings(dl, s, ev); break;
    case Page::About: break;
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

}
