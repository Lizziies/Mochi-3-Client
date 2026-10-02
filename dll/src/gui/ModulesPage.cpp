#include "GuiInternal.hpp"
#include "Gui.hpp"
#include "I18n.hpp"
#include "Icons.hpp"
#include "Theme.hpp"
#include "Tiles.hpp"
#include "Widgets.hpp"
#include "modules/HudModule.hpp"
#include "modules/Manager.hpp"
#include "render/Draw.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"
#include "server/Rules.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <vector>

namespace gui {

static int section = 0;
static const Tile* openTile = nullptr;
static const Tile* shownTile = nullptr;
static Module* shown = nullptr;
static float panelAnim = 0.f;
static ImVec2 panelOrigin{0, 0};
static ImVec2 panelSize{0, 0};
static float gridT = 0.f;
static size_t gridKey = 0;
static std::map<std::string, float> hovers;
static std::map<const Module*, bool> expanded;

int& sectionIndex() { return section; }

void openAllGroups() { section = -1; }

bool panelOpen() { return selectedModule() || openTile; }

void closePanel() {
    selectedModule() = nullptr;
    openTile = nullptr;
}

static void openModule(Module* m) {
    openTile = nullptr;
    selectedModule() = m;
}

static void openGroup(const Tile* t) {
    selectedModule() = nullptr;
    openTile = t;
}

static bool locked(const Module& m) { return !m.available() || m.rule() == RuleLevel::Block; }

static bool isCore(const Setting& st) { return st.id == "key" || st.id == "hold" || st.id == "x" || st.id == "y"; }

static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

static bool matches(const Module& m) {
    if (favoritesOnly() && !m.favorite()) return false;
    if (!searchText()[0]) return true;
    std::string q = lower(searchText());
    if (lower(i18n::tr(m.name().c_str())).find(q) != std::string::npos || lower(m.name()).find(q) != std::string::npos) return true;
    if (lower(i18n::tr(m.description().c_str())).find(q) != std::string::npos) return true;
    if (const Tile* t = tileOf(m); t && t->group && lower(i18n::tr(t->name.c_str())).find(q) != std::string::npos) return true;
    for (auto& tag : m.tags())
        if (lower(tag).find(q) != std::string::npos) return true;
    return false;
}

static void iconChip(ImDrawList* dl, ImVec2 p, float size, Icon kind, bool on, float dim = 1.f) {
    auto& t = theme::current();
    float s = ui::scale();
    ImVec4 bg = on ? theme::mix(t.surfaceHover, t.accent, 0.28f) : t.surfaceHover;
    dl->AddRectFilled(p, p + ImVec2(size, size), theme::col(bg, dim), 8 * s);
    icon(dl, kind, p + ImVec2(size, size) * 0.5f, size * 0.3f, theme::col(on ? t.accent : t.textDim, dim));
}

static void warnMark(ImDrawList* dl, ImVec2 c, float s, float dim) {
    auto& t = theme::current();
    dl->AddTriangleFilled({c.x, c.y - 6 * s}, {c.x - 6 * s, c.y + 5 * s}, {c.x + 6 * s, c.y + 5 * s}, theme::col(t.warn, dim));
    dl->AddText(fonts::bold(), 9.5f * s, {c.x - 1.4f * s, c.y - 3.6f * s}, theme::col(t.bg), "!");
}

static bool closeButton(ImDrawList* dl, const char* id, ImVec2 at, float size) {
    auto& t = theme::current();
    float s = ui::scale();
    ImGui::SetCursorScreenPos(at);
    bool clicked = ImGui::InvisibleButton(id, {size, size});
    bool hov = ImGui::IsItemHovered();
    if (hov) dl->AddRectFilled(at, at + ImVec2(size, size), theme::col(t.surfaceHover), 8 * s);
    ImVec2 c = at + ImVec2(size, size) * 0.5f;
    ImU32 col = theme::col(hov ? t.text : t.textDim);
    dl->AddLine(c + ImVec2(-4 * s, -4 * s), c + ImVec2(4 * s, 4 * s), col, 1.8f * s);
    dl->AddLine(c + ImVec2(4 * s, -4 * s), c + ImVec2(-4 * s, 4 * s), col, 1.8f * s);
    return clicked;
}

static void beginPanel(const char* id, float e) {
    auto& t = theme::current();
    float s = ui::scale();
    ImGui::SetNextWindowPos({panelOrigin.x + (1.f - e) * 32 * s, panelOrigin.y});
    ImGui::SetNextWindowSize(panelSize);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme::col(t.surface, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, theme::col(t.surfaceHover));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10 * s);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {16 * s, 14 * s});
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, e);
    ImGui::Begin(id, nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing);
}

static void endPanel() {
    ImGui::End();
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(2);
}

static bool header(Icon kind, const char* title, const char* subtitle, bool on) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 hp = ImGui::GetCursorScreenPos();
    float full = ImGui::GetContentRegionAvail().x;
    float badge = 42 * s;
    iconChip(dl, hp, badge, kind, on);
    float tx = hp.x + badge + 12 * s;
    float tw = full - badge - 12 * s - 36 * s;
    dl->AddText(fonts::bold(), 19 * s, {tx, hp.y + 1 * s}, theme::col(t.text), fitText(fonts::bold(), 19 * s, title, tw).c_str());
    dl->AddText(fonts::regular(), 12.5f * s, {tx, hp.y + 25 * s}, theme::col(t.textDim),
                fitText(fonts::regular(), 12.5f * s, subtitle, tw).c_str());
    bool clicked = closeButton(dl, "closepane", {hp.x + full - 28 * s, hp.y + 2 * s}, 28 * s);
    ImGui::SetCursorScreenPos(hp + ImVec2(0, badge + 12 * s));
    return clicked;
}

static void keyRow(Module& m) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 cp = ImGui::GetCursorScreenPos();
    float full = ImGui::GetContentRegionAvail().x;
    float h = 40 * s;
    dl->AddRectFilled(cp, cp + ImVec2(full, h), theme::col(t.surfaceHover, 0.5f), 8 * s);
    dl->AddText(fonts::bold(), 14 * s, cp + ImVec2(12 * s, (h - 14 * s) * 0.5f), theme::col(t.text), i18n::tr("Keybind"));
    const char* holdLabel = i18n::tr("Hold");
    ImVec2 hs = ImGui::CalcTextSize(holdLabel);
    float holdX = full - 50 * s;
    float keyX = holdX - hs.x - 18 * s - 112 * s;
    ImGui::SetCursorScreenPos(cp + ImVec2(keyX, (h - ImGui::GetFrameHeight()) * 0.5f));
    widgets::keyCapture("key", m.keybind().i);
    dl->AddText({cp.x + holdX - hs.x - 8 * s, cp.y + (h - hs.y) * 0.5f}, theme::col(t.textDim), holdLabel);
    ImGui::SetCursorScreenPos(cp + ImVec2(holdX, (h - 22 * s) * 0.5f));
    widgets::toggle("hold", m.hold().b);
    ImGui::SetCursorScreenPos(cp + ImVec2(0, h + 10 * s));
}

static void stateBar(Module& m) {
    float s = ui::scale();
    bool lock = locked(m);
    bool on = (m.userEnabled() && !lock) || m.alwaysOn();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float full = ImGui::GetContentRegionAvail().x;
    if (statePill("headerstate", p, p + ImVec2(full, 34 * s), on, lock) && !m.alwaysOn()) m.setEnabled(!on);
    ImGui::SetCursorScreenPos(p + ImVec2(0, 44 * s));
}

static void drawModuleSettings(Module& m, bool grouped) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* hud = dynamic_cast<HudModule*>(&m);
    auto isStyle = [hud](const Setting& st) {
        static const char* ids[] = {"bg", "rounding", "padding", "shadow", "accent", "scale"};
        if (!hud || st.type == SettingType::Color) return false;
        for (auto id : ids)
            if (st.id == id) return true;
        return false;
    };
    auto group = [&](const char* title, auto&& belongs) {
        bool any = false;
        for (auto& set : m.settings())
            if (!isCore(set) && belongs(set) && set.shown()) any = true;
        if (!any) return;
        ImGuiID id = ImGui::GetID(title);
        bool& open = *ImGui::GetStateStorage()->GetBoolRef(id, !grouped || std::string(title) == "General");
        ImVec2 hp = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        bool clicked = ImGui::InvisibleButton(title, {w, 30 * s});
        bool hov = ImGui::IsItemHovered();
        auto* dl = ImGui::GetWindowDrawList();
        if (hov) dl->AddRectFilled(hp, hp + ImVec2(w, 30 * s), theme::col(t.surfaceHover, 0.4f), 8 * s);
        std::string label = i18n::tr(title);
        for (auto& c : label)
            if ((unsigned char)c < 128) c = (char)std::toupper((unsigned char)c);
        dl->AddText(fonts::bold(), 12 * s, hp + ImVec2(8 * s, 9 * s), theme::col(t.textDim), label.c_str());
        ImVec2 ac{hp.x + w - 16 * s, hp.y + 15 * s};
        if (open) dl->AddTriangleFilled(ac + ImVec2(-4 * s, -2.5f * s), ac + ImVec2(4 * s, -2.5f * s), ac + ImVec2(0, 3 * s), theme::col(t.textDim));
        else dl->AddTriangleFilled(ac + ImVec2(-2.5f * s, -4 * s), ac + ImVec2(-2.5f * s, 4 * s), ac + ImVec2(3 * s, 0), theme::col(t.textDim));
        if (clicked) open = !open;
        if (open) {
            ImGui::Dummy({0, 2 * s});
            for (auto& set : m.settings())
                if (!isCore(set) && belongs(set)) widgets::setting(set);
        }
        ImGui::Dummy({0, 6 * s});
    };
    group("General", [&](const Setting& st) { return st.type != SettingType::Color && !isStyle(st); });
    m.drawSettings();
    group("Style", [&](const Setting& st) { return isStyle(st); });
    group("Colors", [&](const Setting& st) { return st.type == SettingType::Color; });
}

static void resetButtons(Module& m) {
    float s = ui::scale();
    if (widgets::button("Reset all", {0, 0}, false))
        m.resetSettings([](const Setting& st) { return st.id != "x" && st.id != "y" && st.id != "key"; });
    if (m.isHud()) {
        ImGui::SameLine();
        if (widgets::button("Reset position", {0, 0}, false))
            m.resetSettings([](const Setting& st) { return st.id == "x" || st.id == "y" || st.id == "scale"; });
    }
    ImGui::Dummy({0, 2 * s});
}

static void ruleText(Module& m) {
    if (m.ruleNote().empty()) return;
    ImGui::PushTextWrapPos(0.f);
    ImGui::TextColored(theme::current().warn, "%s", m.ruleNote().c_str());
    ImGui::PopTextWrapPos();
}

static void drawSettingsPanel(Module& m, float e) {
    float s = ui::scale();
    beginPanel("##mochi_panel", e);
    std::string sub = i18n::tr(m.description().c_str());
    if (const Tile* tile = tileOf(m); tile && tile->group) sub = std::string(i18n::tr(tile->name.c_str())) + "  ·  " + sub;
    if (header(iconOf(m), i18n::tr(m.name().c_str()), sub.c_str(), m.userEnabled() && !locked(m))) selectedModule() = nullptr;
    stateBar(m);
    ruleText(m);
    if (locked(m) && m.rule() != RuleLevel::Block)
        widgets::hint("This one needs game data for your Minecraft version. It turns on by itself once the data is there.");
    ImGui::Dummy({0, 4 * s});
    keyRow(m);

    ImGui::BeginChild("settingsScroll", {0, -(m.isHud() ? 84.f : 44.f) * s}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
    smoothScroll();
    drawModuleSettings(m, false);
    ImGui::EndChild();
    ImGui::Dummy({0, 2 * s});
    resetButtons(m);
    if (m.isHud() && widgets::button("Edit HUD", {ImGui::GetContentRegionAvail().x, 0}, true)) setEditingHud(true);
    endPanel();
}

static void chevron(ImDrawList* dl, ImVec2 c, bool open, ImU32 col) {
    float s = ui::scale();
    if (open) dl->AddTriangleFilled(c + ImVec2(-4 * s, -2.5f * s), c + ImVec2(4 * s, -2.5f * s), c + ImVec2(0, 3 * s), col);
    else dl->AddTriangleFilled(c + ImVec2(-2.5f * s, -4 * s), c + ImVec2(-2.5f * s, 4 * s), c + ImVec2(3 * s, 0), col);
}

static void memberRow(Module& m) {
    auto& t = theme::current();
    float s = ui::scale();
    ImGui::PushID(m.name().c_str());
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    float h = 48 * s;
    bool lock = locked(m);
    bool on = (m.userEnabled() && !lock) || m.alwaysOn();
    bool& open = expanded[&m];

    ImGui::SetNextItemAllowOverlap();
    bool clicked = ImGui::InvisibleButton("row", {w, h});
    bool hov = ImGui::IsItemHovered();
    m.hover = draw::approach(m.hover, hov || open ? 1.f : 0.f, 16.f * t.animSpeed);
    float dim = lock ? 0.45f : 1.f;
    dl->AddRectFilled(p, p + ImVec2(w, h), theme::col(t.surfaceHover, 0.3f + 0.35f * m.hover), 8 * s);
    if (on) dl->AddRectFilled({p.x, p.y + 10 * s}, {p.x + 3 * s, p.y + h - 10 * s}, theme::col(t.accent), 2 * s);
    chevron(dl, {p.x + 18 * s, p.y + h * 0.5f}, open, theme::col(t.textDim));

    float tx = p.x + 34 * s;
    float tw = w - 34 * s - 64 * s;
    const char* name = i18n::tr(m.name().c_str());
    dl->AddText(fonts::bold(), 14.5f * s, {tx, p.y + 8 * s}, theme::col(t.text, dim), fitText(fonts::bold(), 14.5f * s, name, tw).c_str());
    std::string line;
    if (m.rule() == RuleLevel::Block) line = i18n::tr("Blocked on this server");
    else if (lock) line = i18n::tr("Needs game data");
    else if (m.keybind().i) line = i18n::fmt("Key: {}", widgets::keyName(m.keybind().i));
    else line = i18n::tr(m.description().c_str());
    dl->AddText(fonts::regular(), 12 * s, {tx, p.y + 27 * s}, theme::col(t.textDim, dim), fitText(fonts::regular(), 12 * s, line, tw).c_str());
    if (m.risky() || m.rule() == RuleLevel::Warn) {
        ImVec2 ns = fonts::bold()->CalcTextSizeA(14.5f * s, FLT_MAX, 0.f, name);
        warnMark(dl, {std::min(tx + ns.x + 12 * s, tx + tw), p.y + 16 * s}, s, dim);
    }

    ImGui::SetCursorScreenPos({p.x + w - 52 * s, p.y + (h - 22 * s) * 0.5f});
    bool state = on;
    if (widgets::toggle("t", state, !lock && !m.alwaysOn())) m.setEnabled(state);
    bool onToggle = ImGui::IsItemHovered();
    if (clicked && !onToggle) open = !open;

    ImGui::SetCursorScreenPos(p + ImVec2(0, h + 4 * s));
    if (open) {
        ImGui::Indent(10 * s);
        ImGui::Dummy({0, 2 * s});
        widgets::hint(m.description().c_str());
        ruleText(m);
        ImGui::Dummy({0, 4 * s});
        keyRow(m);
        drawModuleSettings(m, true);
        resetButtons(m);
        ImGui::Unindent(10 * s);
        ImGui::Dummy({0, 6 * s});
    }
    ImGui::PopID();
}

static void drawGroupPanel(const Tile& tile, float e) {
    auto& t = theme::current();
    float s = ui::scale();
    beginPanel("##mochi_group", e);
    int on = tile.enabled();
    if (header(tile.icon, i18n::tr(tile.name.c_str()), i18n::tr(tile.blurb.c_str()), on > 0)) openTile = nullptr;
    {
        auto* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        std::string line = i18n::fmt("{} of {} on", on, tile.members.size());
        if (tile.usable() < (int)tile.members.size()) line += "  ·  " + i18n::fmt("{} need game data", tile.members.size() - tile.usable());
        dl->AddText(fonts::regular(), 13 * s, p, theme::col(t.textDim), line.c_str());
        ImGui::Dummy({0, 22 * s});
    }
    ImGui::BeginChild("members", {0, tile.hud() ? -44.f * s : 0.f}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
    smoothScroll();
    for (auto* m : tile.members) memberRow(*m);
    ImGui::Dummy({0, 8 * s});
    ImGui::EndChild();
    if (tile.hud() && widgets::button("Edit HUD", {ImGui::GetContentRegionAvail().x, 0}, true)) setEditingHud(true);
    endPanel();
}

struct Entry {
    const Tile* tile;
    Module* solo;
};

static std::string tileLine(const Tile& tile, Module* solo, bool lock, bool on) {
    if (!solo) return lock ? std::string(i18n::tr("Needs game data")) : i18n::fmt("{} of {} on", tile.enabled(), tile.members.size());
    if (solo->rule() == RuleLevel::Block) return i18n::tr("Blocked here");
    if (lock) return i18n::tr("Needs game data");
    if (solo->keybind().i) return i18n::fmt("{}  ·  {}", i18n::tr(on ? "On" : "Off"), widgets::keyName(solo->keybind().i));
    return i18n::tr(on ? "On" : "Off");
}

static void drawTile(const Entry& entry, ImVec2 p, ImVec2 size, float appear) {
    auto& t = theme::current();
    float s = ui::scale();
    const Tile& tile = *entry.tile;
    Module* solo = entry.solo ? entry.solo : (tile.group ? nullptr : tile.members.front());
    std::string key = solo ? solo->name() : tile.name;
    ImGui::PushID(key.c_str());

    bool lock = solo ? locked(*solo) : tile.usable() == 0;
    bool on = solo ? ((solo->userEnabled() && !lock) || solo->alwaysOn()) : tile.enabled() > 0;
    bool selected = solo ? selectedModule() == solo : openTile == &tile;
    float dim = lock ? 0.5f : 1.f;
    float base = theme::fade();
    theme::setFade(base * appear);

    ImGui::SetCursorScreenPos(p);
    ImGui::SetNextItemAllowOverlap();
    bool clicked = ImGui::InvisibleButton("tile", size);
    bool hov = ImGui::IsItemHovered();
    bool rclick = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    float& h = hovers[key];
    h = draw::approach(h, hov || selected ? 1.f : 0.f, 16.f * t.animSpeed);

    auto* dl = ImGui::GetWindowDrawList();
    float r = 10 * s;
    ImVec2 a = p - ImVec2(0, h * 2 * s), b = a + size;
    ImVec4 bg = theme::mix(t.surface, t.surfaceHover, 0.55f * h);
    if (on) bg = theme::mix(bg, t.accent, 0.1f);
    dl->AddRectFilled(a, b, theme::col(bg), r);
    ImVec4 edge = on ? t.accent : (selected ? t.textDim : t.surfaceHover);
    dl->AddRect(a, b, theme::col(edge, on ? 0.85f : 0.9f), r, 0, (on || selected ? 1.5f : 1.f) * s);

    float pad = 12 * s;
    Icon kind = solo && entry.solo ? iconOf(*solo) : tile.icon;
    iconChip(dl, a + ImVec2(pad, pad), 34 * s, kind, on, dim);

    float tx = a.x + pad;
    float tw = size.x - pad * 2;
    const char* name = i18n::tr(solo ? solo->name().c_str() : tile.name.c_str());
    dl->AddText(fonts::bold(), 15 * s, {tx, a.y + 54 * s}, theme::col(t.text, dim), fitText(fonts::bold(), 15 * s, name, tw).c_str());
    ImU32 lineCol = on ? theme::col(t.accent, dim) : theme::col(t.textDim, dim);
    dl->AddText(fonts::regular(), 12 * s, {tx, a.y + 74 * s}, lineCol, fitText(fonts::regular(), 12 * s, tileLine(tile, solo, lock, on), tw).c_str());

    float cx = b.x - pad - 22 * s;
    bool gearClicked = false;
    bool starClicked = false;
    if (solo) {
        ImGui::SetCursorScreenPos({cx, a.y + pad});
        gearClicked = ImGui::InvisibleButton("gear", {22 * s, 22 * s});
        bool gh = ImGui::IsItemHovered();
        if (gh) dl->AddRectFilled({cx, a.y + pad}, {cx + 22 * s, a.y + pad + 22 * s}, theme::col(t.surfaceHover), 6 * s);
        if (hov || gh || selected) icon(dl, Icon::Gear, {cx + 11 * s, a.y + pad + 11 * s}, 7 * s, theme::col(gh ? t.text : t.textDim));

        ImVec2 sc{cx - 14 * s, a.y + pad + 11 * s};
        ImGui::SetCursorScreenPos(sc - ImVec2(10 * s, 10 * s));
        starClicked = ImGui::InvisibleButton("fav", {20 * s, 20 * s});
        bool sh = ImGui::IsItemHovered();
        if (solo->favorite()) star(dl, sc, 6.5f * s, theme::col({1.f, 0.82f, 0.4f, 1.f}, dim), true);
        else if (hov || sh) star(dl, sc, 6.5f * s, theme::col(sh ? t.text : t.textDim, 0.7f), false);
        if (starClicked) solo->setFavorite(!solo->favorite());
    } else {
        icon(dl, Icon::Grid, {cx + 11 * s, a.y + pad + 11 * s}, 6 * s, theme::col(t.textDim, 0.8f * dim));
    }
    bool warn = solo ? (solo->risky() || solo->rule() == RuleLevel::Warn) : tile.risky();
    if (warn) warnMark(dl, {a.x + pad + 40 * s, a.y + pad + 5 * s}, s, dim);
    if (lock) icon(dl, Icon::Lock, {b.x - pad - 4 * s, b.y - pad - 4 * s}, 5 * s, theme::col(t.textDim, 0.7f));
    if (on) dl->AddRectFilled({a.x + pad, b.y - 3 * s}, {b.x - pad, b.y - 1 * s}, theme::col(t.accent, 0.9f), 1 * s);

    if (hov && lock) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(solo && solo->rule() == RuleLevel::Block ? solo->ruleNote().c_str()
                                                                         : i18n::tr("Needs game data for this Minecraft version."));
        ImGui::EndTooltip();
    }

    if (gearClicked || rclick) {
        if (solo) openModule(solo);
        else openGroup(&tile);
    } else if (clicked && !starClicked) {
        if (!solo) openGroup(&tile);
        else if (!lock && !solo->alwaysOn()) solo->setEnabled(!solo->userEnabled());
        else openModule(solo);
    }

    theme::setFade(base);
    ImGui::PopID();
}

static void sectionHeader(const char* title, float w) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    std::string label = title;
    for (auto& c : label)
        if ((unsigned char)c < 128) c = (char)std::toupper((unsigned char)c);
    dl->AddText(fonts::bold(), 12 * s, {p.x + 2 * s, p.y + 4 * s}, theme::col(t.textDim), label.c_str());
    ImVec2 ts = fonts::bold()->CalcTextSizeA(12 * s, FLT_MAX, 0.f, label.c_str());
    dl->AddLine({p.x + ts.x + 12 * s, p.y + 11 * s}, {p.x + w, p.y + 11 * s}, theme::col(t.surfaceHover), 1.f);
    ImGui::Dummy({w, 26 * s});
}

using Block = std::pair<std::string, std::vector<Entry>>;

static std::vector<Block> blocks() {
    std::vector<Block> out;
    if (searchText()[0] || favoritesOnly()) {
        std::vector<Entry> list;
        for (auto& t : tiles())
            for (auto* m : t.members)
                if (matches(*m)) list.push_back({&t, m});
        std::string title = favoritesOnly() && !searchText()[0] ? i18n::tr("Favorites") : i18n::tr("Results");
        if (!list.empty()) out.push_back({title, std::move(list)});
        return out;
    }
    for (int i = 0; i < sectionCount; i++) {
        if (section >= 0 && section != i) continue;
        std::vector<Entry> list;
        for (auto& t : tiles())
            if (int(t.section) == i) list.push_back({&t, nullptr});
        if (!list.empty()) out.push_back({sectionName(Section(i)), std::move(list)});
    }
    return out;
}

void drawModulesPage(ImVec2 origin, ImVec2 size) {
    auto& t = theme::current();
    float s = ui::scale();
    if (selectedModule()) {
        shown = selectedModule();
        shownTile = nullptr;
    } else if (openTile) {
        shownTile = openTile;
        shown = nullptr;
    }
    panelAnim = draw::approach(panelAnim, panelOpen() ? 1.f : 0.f, 14.f * t.animSpeed);

    float gap = 14 * s;
    float panelW = std::clamp(size.x * 0.42f, 340 * s, 460 * s);
    float gridW = panelOpen() ? size.x - panelW - gap : size.x;
    panelOrigin = {origin.x + size.x - panelW, origin.y};
    panelSize = {panelW, size.y};

    size_t key = std::hash<std::string>{}(searchText()) * 31 + (favoritesOnly() ? 1 : 0) + size_t(section + 2) * 7919;
    if (key != gridKey) {
        gridKey = key;
        gridT = 0.f;
    }
    gridT += ui::dt();

    ImGui::SetCursorScreenPos(origin);
    ImGui::BeginChild("grid", {gridW, size.y}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
    smoothScroll();
    auto list = blocks();
    if (list.empty()) widgets::hint(favoritesOnly() && !searchText()[0] ? "No favorites yet. Hover a tile and click its star." : "Nothing found.");

    float w = ImGui::GetContentRegionAvail().x - 6 * s;
    float tileGap = 10 * s;
    int cols = std::max(1, int((w + tileGap) / (172 * s + tileGap)));
    float tileW = (w - tileGap * float(cols - 1)) / float(cols);
    float tileH = 98 * s;
    int index = 0;
    bool headers = list.size() > 1 || searchText()[0] || favoritesOnly();
    for (auto& [title, items] : list) {
        if (headers) sectionHeader(title.c_str(), w);
        ImVec2 start = ImGui::GetCursorScreenPos() + ImVec2(0, 3 * s);
        for (size_t i = 0; i < items.size(); i++) {
            int col = int(i) % cols, row = int(i) / cols;
            ImVec2 p = start + ImVec2(float(col) * (tileW + tileGap), float(row) * (tileH + tileGap));
            float e = draw::motion() ? draw::easeOutCubic(std::clamp((gridT - float(index) * 0.012f) / 0.25f, 0.f, 1.f)) : 1.f;
            drawTile(items[i], p + ImVec2(0, (1.f - e) * 8 * s), {tileW, tileH}, e);
            index++;
        }
        int rows = (int(items.size()) + cols - 1) / cols;
        ImGui::SetCursorScreenPos(start + ImVec2(0, float(rows) * (tileH + tileGap) + 8 * s));
        ImGui::Dummy({w, 1});
    }
    ImGui::Dummy({0, 10 * s});
    ImGui::EndChild();
}

void drawModulePanel() {
    float e = draw::easeOutCubic(panelAnim);
    if (panelAnim > 0.01f && shown) drawSettingsPanel(*shown, e);
    else if (panelAnim > 0.01f && shownTile) drawGroupPanel(*shownTile, e);
    else {
        shown = nullptr;
        shownTile = nullptr;
    }
    if (panelOpen() && ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !widgets::capturingKey()) closePanel();
}

}
