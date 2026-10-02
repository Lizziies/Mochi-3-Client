#include "Catalog.hpp"
#include "GuiInternal.hpp"
#include "Gui.hpp"
#include "I18n.hpp"
#include "Theme.hpp"
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

static std::map<const void*, bool> opened;
static std::map<const void*, float> expandT;
static std::map<const void*, float> heights;
static std::map<int, bool> folded{{int(Section::Server), true}, {int(Section::Extras), true}};
static const void* scrollTo = nullptr;
static double shownAt = 0.0;
static bool wasOpen = false;

void openAllGroups() { folded.clear(); }

static bool isCore(const Setting& st) { return st.id == "key" || st.id == "hold" || st.id == "x" || st.id == "y"; }

static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

static std::string upper(std::string s) {
    for (auto& c : s)
        if ((unsigned char)c < 128) c = (char)std::toupper((unsigned char)c);
    return s;
}

static bool matches(const Module& m) {
    if (favoritesOnly() && !m.favorite()) return false;
    if (!searchText()[0]) return true;
    std::string q = lower(searchText());
    if (lower(i18n::tr(m.name().c_str())).find(q) != std::string::npos || lower(m.name()).find(q) != std::string::npos) return true;
    if (lower(i18n::tr(m.description().c_str())).find(q) != std::string::npos) return true;
    if (const Entry* e = entryOf(m); e && e->group && lower(i18n::tr(e->name.c_str())).find(q) != std::string::npos) return true;
    for (auto& tag : m.tags())
        if (lower(tag).find(q) != std::string::npos) return true;
    return false;
}

static void chevron(ImDrawList* dl, ImVec2 c, float open, ImU32 col) {
    float s = ui::scale();
    float a = open * 1.5708f;
    auto rot = [&](float x, float y) { return ImVec2(c.x + (x * std::cos(a) - y * std::sin(a)) * s, c.y + (x * std::sin(a) + y * std::cos(a)) * s); };
    dl->AddTriangleFilled(rot(-2.5f, -4.f), rot(-2.5f, 4.f), rot(3.5f, 0.f), col);
}

static void warnMark(ImDrawList* dl, ImVec2 c, float dim) {
    auto& t = theme::current();
    float s = ui::scale();
    dl->AddTriangleFilled({c.x, c.y - 6 * s}, {c.x - 6 * s, c.y + 5 * s}, {c.x + 6 * s, c.y + 5 * s}, theme::col(t.warn, dim));
    dl->AddText(fonts::bold(), 9.5f * s, {c.x - 1.4f * s, c.y - 3.6f * s}, theme::col(t.bg), "!");
}

struct Row {
    std::string name;
    std::string extra;
    bool on = false;
    bool lock = false;
    bool warn = false;
    bool canToggle = true;
    int key = 0;
    Module* fav = nullptr;
};

// returns true when the row itself (not its switch or star) was clicked
static bool drawRow(const void* id, const Row& row, float indent, float h, float appear, bool& flipped) {
    auto& t = theme::current();
    float s = ui::scale();
    ImGui::PushID(id);
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImVec2 p = origin + ImVec2(indent, 0);
    float w = ImGui::GetContentRegionAvail().x - indent;
    ImGui::SetCursorScreenPos(p);
    ImGui::SetNextItemAllowOverlap();
    bool clicked = ImGui::InvisibleButton("row", {w, h});
    bool hov = ImGui::IsItemHovered();
    float open = draw::easeOutCubic(expandT[id]);
    float& hv = *ImGui::GetStateStorage()->GetFloatRef(ImGui::GetID("hv"), 0.f);
    hv = draw::approach(hv, hov ? 1.f : 0.f, 18.f * t.animSpeed);

    float base = theme::fade();
    theme::setFade(base * appear);
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 a = p + ImVec2((1.f - appear) * 14 * s, 0), b = a + ImVec2(w, h);
    float r = t.rounding * s;
    float bg = std::max(0.55f * hv, 0.35f * open);
    if (bg > 0.01f) dl->AddRectFilled(a, b, theme::col(t.surfaceHover, bg), r);
    if (row.on) dl->AddRectFilled({a.x, a.y + 9 * s}, {a.x + 2.5f * s, b.y - 9 * s}, theme::col(t.accent), 1 * s);

    float dim = row.lock ? 0.45f : 1.f;
    float cy = a.y + h * 0.5f;
    chevron(dl, {a.x + 14 * s, cy}, open, theme::col(t.textDim, 0.8f * dim));
    float tx = a.x + 28 * s;
    float size = (indent > 0 ? 15.f : 16.f) * s;
    ImFont* font = row.on ? fonts::bold() : fonts::regular();
    float right = b.x - 52 * s;
    std::string label = fitText(font, size, row.name, right - tx - 60 * s);
    dl->AddText(font, size, {tx, cy - size * 0.55f}, theme::col(row.on ? t.text : theme::mix(t.textDim, t.text, 0.75f), dim), label.c_str());
    float after = tx + font->CalcTextSizeA(size, FLT_MAX, 0.f, label.c_str()).x + 8 * s;
    if (!row.extra.empty()) dl->AddText(fonts::regular(), 13 * s, {after, cy - 7 * s}, theme::col(t.textDim, 0.8f * dim), row.extra.c_str());

    float x = right;
    if (row.lock) {
        const char* text = i18n::tr("No game data");
        ImVec2 ts = fonts::regular()->CalcTextSizeA(12.5f * s, FLT_MAX, 0.f, text);
        x -= ts.x + 8 * s;
        dl->AddText(fonts::regular(), 12.5f * s, {x, cy - 7 * s}, theme::col(t.textDim, 0.5f), text);
    } else if (row.key) {
        std::string k = widgets::keyName(row.key);
        ImVec2 ks = fonts::regular()->CalcTextSizeA(12.5f * s, FLT_MAX, 0.f, k.c_str());
        float kw = ks.x + 12 * s;
        x -= kw + 8 * s;
        dl->AddRect({x, cy - 9 * s}, {x + kw, cy + 9 * s}, theme::col(theme::border()), r * 0.7f, 0, 1.f);
        dl->AddText(fonts::regular(), 12.5f * s, {x + 6 * s, cy - ks.y * 0.5f}, theme::col(t.textDim), k.c_str());
    }
    if (row.warn) {
        x -= 18 * s;
        warnMark(dl, {x + 6 * s, cy}, dim);
    }
    bool starClicked = false;
    if (row.fav) {
        ImVec2 sc{x - 12 * s, cy};
        ImGui::SetCursorScreenPos(sc - ImVec2(9 * s, 9 * s));
        starClicked = ImGui::InvisibleButton("fav", {18 * s, 18 * s});
        bool sh = ImGui::IsItemHovered();
        if (row.fav->favorite()) star(dl, sc, 6 * s, theme::col({1.f, 0.82f, 0.4f, 1.f}, dim), true);
        else if (hov || sh) star(dl, sc, 6 * s, theme::col(sh ? t.text : t.textDim, 0.7f), false);
        if (starClicked) row.fav->setFavorite(!row.fav->favorite());
    }

    ImGui::SetCursorScreenPos({b.x - 46 * s, cy - 11 * s});
    bool state = row.on;
    flipped = widgets::toggle("t", state, row.canToggle);
    bool onSwitch = ImGui::IsItemHovered();

    theme::setFade(base);
    ImGui::SetCursorScreenPos(origin + ImVec2(0, h + 2 * s));
    ImGui::PopID();
    return clicked && !onSwitch && !starClicked;
}

static void keyRow(Module& m) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 cp = ImGui::GetCursorScreenPos();
    float full = ImGui::GetContentRegionAvail().x;
    float h = 36 * s;
    dl->AddRect(cp, cp + ImVec2(full, h), theme::col(theme::border()), t.rounding * s, 0, 1.f);
    dl->AddText(fonts::regular(), 14 * s, cp + ImVec2(10 * s, (h - 14 * s) * 0.5f), theme::col(t.text), i18n::tr("Keybind"));
    const char* holdLabel = i18n::tr("Hold");
    ImVec2 hs = fonts::regular()->CalcTextSizeA(13 * s, FLT_MAX, 0.f, holdLabel);
    float holdX = full - 48 * s;
    float keyX = holdX - hs.x - 16 * s - 110 * s;
    ImGui::SetCursorScreenPos(cp + ImVec2(keyX, (h - ImGui::GetFrameHeight()) * 0.5f));
    widgets::keyCapture("key", m.keybind().i);
    dl->AddText(fonts::regular(), 13 * s, {cp.x + holdX - hs.x - 8 * s, cp.y + (h - hs.y) * 0.5f}, theme::col(t.textDim), holdLabel);
    ImGui::SetCursorScreenPos(cp + ImVec2(holdX, (h - 22 * s) * 0.5f));
    widgets::toggle("hold", m.hold().b);
    ImGui::SetCursorScreenPos(cp + ImVec2(0, h + 8 * s));
}

static void settingGroups(Module& m) {
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
    auto group = [&](const char* title, bool startOpen, auto&& belongs) {
        bool any = false;
        for (auto& set : m.settings())
            if (!isCore(set) && belongs(set) && set.shown()) any = true;
        if (!any) return;
        bool& open = *ImGui::GetStateStorage()->GetBoolRef(ImGui::GetID(title), startOpen);
        ImVec2 hp = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        bool clicked = ImGui::InvisibleButton(title, {w, 26 * s});
        auto* dl = ImGui::GetWindowDrawList();
        chevron(dl, {hp.x + 6 * s, hp.y + 13 * s}, open ? 1.f : 0.f, theme::col(t.textDim));
        dl->AddText(fonts::bold(), 12 * s, hp + ImVec2(16 * s, 6 * s), theme::col(t.textDim), upper(i18n::tr(title)).c_str());
        if (clicked) open = !open;
        if (open)
            for (auto& set : m.settings())
                if (!isCore(set) && belongs(set)) widgets::setting(set);
        ImGui::Dummy({0, 4 * s});
    };
    group("General", true, [&](const Setting& st) { return st.type != SettingType::Color && !isStyle(st); });
    m.drawSettings();
    group("Style", false, [&](const Setting& st) { return isStyle(st); });
    group("Colors", false, [&](const Setting& st) { return st.type == SettingType::Color; });
}

static void smallHint(const char* text) {
    ImGui::PushFont(fonts::regular(), 15.f);
    widgets::hint(text);
    ImGui::PopFont();
}

static void moduleBody(Module& m) {
    auto& t = theme::current();
    float s = ui::scale();
    ImGui::Dummy({0, 2 * s});
    smallHint(m.description().c_str());
    if (!m.ruleNote().empty()) {
        ImGui::PushTextWrapPos(0.f);
        ImGui::TextColored(t.warn, "%s", m.ruleNote().c_str());
        ImGui::PopTextWrapPos();
    }
    if (locked(m) && m.rule() != RuleLevel::Block)
        smallHint("This one needs game data for your Minecraft version. It turns on by itself once the data is there.");
    ImGui::Dummy({0, 4 * s});
    keyRow(m);
    settingGroups(m);
    if (widgets::button("Reset all", {0, 0}, false))
        m.resetSettings([](const Setting& st) { return st.id != "x" && st.id != "y" && st.id != "key"; });
    if (m.isHud()) {
        ImGui::SameLine();
        if (widgets::button("Reset position", {0, 0}, false))
            m.resetSettings([](const Setting& st) { return st.id == "x" || st.id == "y" || st.id == "scale"; });
        ImGui::SameLine();
        if (widgets::button("Edit HUD", {0, 0}, true)) setEditingHud(true);
    }
    ImGui::Dummy({0, 8 * s});
}

// Draws the body below a row in a child whose height follows the open animation, so rows below slide
// instead of jumping. The height is the content measured in the previous frame.
template <class F>
static void expander(const void* id, float indent, F&& body) {
    auto& t = theme::current();
    float& a = expandT[id];
    a = draw::motion() ? draw::approach(a, opened[id] ? 1.f : 0.f, 15.f * t.animSpeed) : (opened[id] ? 1.f : 0.f);
    if (a < 0.002f) return;
    float full = heights[id];
    float h = std::max(1.f, full * draw::easeOutCubic(a));
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(p + ImVec2(indent, 0));
    ImGui::PushID(id);
    ImGui::BeginChild("body", {ImGui::GetContentRegionAvail().x - indent, h}, 0,
                      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    float base = theme::fade();
    theme::setFade(base * std::min(1.f, a * 1.4f));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * std::min(1.f, a * 1.4f));
    body();
    ImGui::PopStyleVar();
    theme::setFade(base);
    heights[id] = ImGui::GetCursorPosY();
    ImGui::EndChild();
    ImGui::PopID();
}

static Row rowFor(Module& m, const std::string& extra) {
    Row row;
    bool lock = locked(m);
    row.name = i18n::tr(m.name().c_str());
    row.extra = extra;
    row.on = (m.userEnabled() && !lock) || m.alwaysOn();
    row.lock = lock;
    row.warn = m.risky() || m.rule() == RuleLevel::Warn;
    row.canToggle = !lock && !m.alwaysOn();
    row.key = m.keybind().i;
    row.fav = &m;
    return row;
}

static float appearAt(int index) {
    if (!draw::motion()) return 1.f;
    float t = float(ui::time() - shownAt) - index * 0.014f;
    return draw::easeOutCubic(std::clamp(t / 0.22f, 0.f, 1.f));
}

static void moduleItem(Module& m, const std::string& extra, float indent, int index) {
    float s = ui::scale();
    bool flipped = false;
    if (drawRow(&m, rowFor(m, extra), indent, (indent > 0 ? 34.f : 38.f) * s, appearAt(index), flipped)) opened[&m] = !opened[&m];
    if (flipped) m.setEnabled(!m.userEnabled());
    if (scrollTo == &m) {
        ImGui::SetScrollHereY(0.2f);
        scrollTo = nullptr;
    }
    expander(&m, indent + 14 * ui::scale(), [&] { moduleBody(m); });
}

static void entryItem(const Entry& e, int index) {
    float s = ui::scale();
    if (!e.group) {
        moduleItem(*e.members.front(), "", 0.f, index);
        return;
    }
    Row row;
    row.name = i18n::tr(e.name.c_str());
    row.extra = i18n::fmt("{}/{}", e.enabled(), e.members.size());
    row.on = e.on();
    row.lock = e.usable() == 0;
    row.warn = e.risky();
    row.canToggle = !row.lock;
    bool flipped = false;
    if (drawRow(&e, row, 0.f, 38 * s, appearAt(index), flipped)) opened[&e] = !opened[&e];
    if (flipped) e.toggle();
    if (scrollTo == &e) {
        ImGui::SetScrollHereY(0.2f);
        scrollTo = nullptr;
    }
    expander(&e, 0.f, [&] {
        if (!e.blurb.empty()) {
            ImGui::Indent(28 * s);
            smallHint(e.blurb.c_str());
            ImGui::Unindent(28 * s);
            ImGui::Dummy({0, 2 * s});
        }
        for (auto* m : e.members) moduleItem(*m, "", 14 * s, 0);
        ImGui::Dummy({0, 6 * s});
    });
}

static bool sectionHeader(int id, const char* title, int on) {
    auto& t = theme::current();
    float s = ui::scale();
    ImGui::PushID(id);
    ImVec2 p = ImGui::GetCursorScreenPos() + ImVec2(0, 6 * s);
    float w = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorScreenPos(p);
    bool clicked = ImGui::InvisibleButton("sec", {w, 28 * s});
    bool hov = ImGui::IsItemHovered();
    ImGui::PopID();
    bool& fold = folded[id];
    if (clicked) fold = !fold;
    auto* dl = ImGui::GetWindowDrawList();
    float cy = p.y + 14 * s;
    std::string label = upper(title);
    dl->AddText(fonts::bold(), 13 * s, {p.x + 8 * s, cy - 7 * s}, theme::col(hov ? t.text : t.textDim), label.c_str());
    float lx = p.x + 8 * s + fonts::bold()->CalcTextSizeA(13 * s, FLT_MAX, 0.f, label.c_str()).x + 10 * s;
    std::string count = i18n::fmt("{} on", on);
    ImVec2 cs = fonts::regular()->CalcTextSizeA(12 * s, FLT_MAX, 0.f, count.c_str());
    float cx = p.x + w - cs.x - 24 * s;
    dl->AddLine({lx, cy}, {cx - 10 * s, cy}, theme::col(theme::border()), 1.f);
    dl->AddText(fonts::regular(), 12 * s, {cx, cy - 7 * s}, theme::col(t.textDim, on ? 1.f : 0.6f), count.c_str());
    chevron(dl, {p.x + w - 10 * s, cy}, fold ? 0.f : 1.f, theme::col(t.textDim));
    ImGui::SetCursorScreenPos(p + ImVec2(0, 30 * s));
    return !fold;
}

static void reveal(Module* m) {
    const Entry* e = entryOf(*m);
    if (!e) return;
    folded[int(e->section)] = false;
    opened[e] = true;
    opened[m] = true;
    scrollTo = e->group ? static_cast<const void*>(e) : static_cast<const void*>(m);
}

void drawModulesPage(ImVec2 origin, ImVec2 size) {
    float s = ui::scale();
    if (open() && !wasOpen) shownAt = ui::time();
    wasOpen = open();
    if (Module*& want = selectedModule()) {
        reveal(want);
        want = nullptr;
    }
    static bool onServer = false;
    bool nowServer = !rules::status().server.empty();
    if (nowServer != onServer) {
        onServer = nowServer;
        folded[int(Section::Server)] = !nowServer;
    }

    static std::string lastQuery;
    std::string query = std::string(searchText()) + (favoritesOnly() ? "*" : "");
    bool jumpTop = query != lastQuery;
    lastQuery = query;

    ImGui::SetCursorScreenPos(origin);
    ImGui::BeginChild("list", size, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
    if (jumpTop) ImGui::SetScrollY(0.f);
    smoothScroll();
    int index = 0;
    if (searchText()[0] || favoritesOnly()) {
        bool any = false;
        for (auto& e : catalog())
            for (auto* m : e.members) {
                if (!matches(*m)) continue;
                any = true;
                moduleItem(*m, e.group ? std::string(i18n::tr(e.name.c_str())) : std::string(), 0.f, index++);
            }
        if (!any) widgets::hint(favoritesOnly() && !searchText()[0] ? "No favorites yet. Click the star next to a module." : "Nothing found.");
    } else {
        bool favs = false;
        for (auto& m : modules::all())
            if (m->favorite() && m->category() != Category::Client) favs = true;
        ImGui::PushID("favorites");
        if (favs && sectionHeader(100, i18n::tr("Favorites"), 0))
            for (auto& m : modules::all())
                if (m->favorite() && m->category() != Category::Client) moduleItem(*m, "", 0.f, index++);
        ImGui::PopID();
        for (int i = 0; i < sectionCount; i++) {
            int on = 0, total = 0;
            for (auto& e : catalog())
                if (int(e.section) == i) {
                    on += e.enabled();
                    total++;
                }
            if (!total || !sectionHeader(i, sectionName(Section(i)), on)) continue;
            for (auto& e : catalog())
                if (int(e.section) == i) entryItem(e, index++);
        }
    }
    ImGui::Dummy({0, 14 * s});
    ImGui::EndChild();
}

}
