#include "GuiInternal.hpp"
#include "Gui.hpp"
#include "I18n.hpp"
#include "Theme.hpp"
#include "Widgets.hpp"
#include "modules/HudModule.hpp"
#include "modules/Manager.hpp"
#include "modules/Tiers.hpp"
#include "server/Rules.hpp"
#include "render/Draw.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iterator>
#include <map>
#include <vector>

namespace gui {

static float listT = 0.f;
static size_t listKey = 0;
static float panelAnim = 0.f;
static Module* shown = nullptr;
static ImVec2 panelOrigin{0, 0};
static ImVec2 panelSize{0, 0};

static void drawSettingsPanel(ImVec2 origin, ImVec2 size, Module& m, float e) {
    auto& t = theme::current();
    float s = ui::scale();
    ImGui::SetNextWindowPos({origin.x + (1.f - e) * 40 * s, origin.y});
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme::col(t.surface, 0.97f));
    ImGui::PushStyleColor(ImGuiCol_Border, theme::col(t.surfaceHover, 0.9f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, t.rounding * s);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {18 * s, 16 * s});
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, e);
    ImGui::Begin("##mochi_panel", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing);

    {
        auto* dl = ImGui::GetWindowDrawList();
        ImVec2 hp = ImGui::GetCursorScreenPos();
        float badge = 46 * s;
        ImVec4 cc = catColor((int)catOf(m));
        draw::gradientRect(dl, hp, hp + ImVec2(badge, badge), theme::col(cc), theme::col(theme::mix(cc, t.accent2, 0.5f)), 12 * s);
        glyph(dl, (int)catOf(m), hp + ImVec2(badge, badge) * 0.5f, 10 * s, theme::col(t.bg, 0.9f));
        dl->AddText(fonts::bold(), 22 * s, hp + ImVec2(badge + 14 * s, 1 * s), theme::col(t.text), i18n::tr(m.name().c_str()));
        std::string cat = categoryName(catOf(m));
        if (!m.sub().empty()) cat += "  ·  " + std::string(i18n::tr(m.sub().c_str()));
        dl->AddText(fonts::regular(), 13.5f * s, hp + ImVec2(badge + 14 * s, 30 * s), theme::col(t.textDim), cat.c_str());

        ImVec2 xp{hp.x + ImGui::GetContentRegionAvail().x - 28 * s, hp.y + 2 * s};
        ImGui::SetCursorScreenPos(xp);
        bool closeClicked = ImGui::InvisibleButton("closepane", {28 * s, 28 * s});
        bool hov = ImGui::IsItemHovered();
        if (hov) dl->AddRectFilled(xp, xp + ImVec2(28 * s, 28 * s), theme::col(t.surfaceHover), 14 * s);
        ImVec2 cx = xp + ImVec2(14 * s, 14 * s);
        ImU32 xc = theme::col(hov ? t.text : t.textDim);
        dl->AddLine(cx + ImVec2(-4 * s, -4 * s), cx + ImVec2(4 * s, 4 * s), xc, 1.8f * s);
        dl->AddLine(cx + ImVec2(4 * s, -4 * s), cx + ImVec2(-4 * s, 4 * s), xc, 1.8f * s);
        if (closeClicked) selectedModule() = nullptr;
        ImGui::SetCursorScreenPos(hp + ImVec2(0, badge + 8 * s));
        bool locked = !m.available() || m.rule() == RuleLevel::Block;
        bool on = (m.userEnabled() && !locked) || m.alwaysOn();
        float full = ImGui::GetContentRegionAvail().x;
        if (statePill("headerstate", ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + ImVec2(full, 34 * s), on, locked) && !m.alwaysOn())
            m.setEnabled(!on);
        ImGui::SetCursorScreenPos(hp + ImVec2(0, badge + 8 * s + 42 * s));
    }
    widgets::hint(m.description().c_str());
    if (!m.ruleNote().empty()) ImGui::TextColored(t.warn, "%s", m.ruleNote().c_str());
    ImGui::Dummy({0, 6 * s});

    auto* hud = dynamic_cast<HudModule*>(&m);
    auto isStyle = [hud](const Setting& st) {
        static const char* ids[] = {"bg", "rounding", "padding", "shadow", "accent", "scale"};
        if (!hud || st.type == SettingType::Color) return false;
        for (auto id : ids)
            if (st.id == id) return true;
        return false;
    };
    auto isCore = [](const Setting& st) { return st.id == "key" || st.id == "hold" || st.id == "x" || st.id == "y"; };

    {
        ImGui::BeginChild("top", {0, 58 * s}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
        auto* dl = ImGui::GetWindowDrawList();
        ImVec2 cp = ImGui::GetCursorScreenPos();
        float full = ImGui::GetContentRegionAvail().x;
        float half = (full - 10 * s) * 0.5f;
        float h = 52 * s;
        dl->AddRectFilled(cp, cp + ImVec2(half, h), theme::col(t.surfaceHover, 0.55f), 12 * s);
        dl->AddText(fonts::bold(), 14.5f * s, cp + ImVec2(12 * s, 8 * s), theme::col(t.text), i18n::tr("Hold mode"));
        dl->AddText(fonts::regular(), 12.f * s, cp + ImVec2(12 * s, 28 * s), theme::col(t.textDim),
                    fitText(fonts::regular(), 12.f * s, i18n::tr("Only on while the key is held"), half - 74 * s).c_str());
        ImGui::SetCursorScreenPos(cp + ImVec2(half - 54 * s, 14 * s));
        widgets::toggle("hold", m.hold().b);

        ImVec2 kp = cp + ImVec2(half + 10 * s, 0);
        dl->AddRectFilled(kp, kp + ImVec2(half, h), theme::col(t.surfaceHover, 0.55f), 12 * s);
        dl->AddText(fonts::bold(), 14.5f * s, kp + ImVec2(12 * s, 8 * s), theme::col(t.text), i18n::tr("Keybind"));
        dl->AddText(fonts::regular(), 12.f * s, kp + ImVec2(12 * s, 28 * s), theme::col(t.textDim),
                    fitText(fonts::regular(), 12.f * s, i18n::tr("Click, then press a key"), half - 130 * s).c_str());
        ImGui::SetCursorScreenPos(kp + ImVec2(half - 112 * s, 12 * s));
        widgets::keyCapture("key", m.keybind().i);
        ImGui::EndChild();
    }

    ImGui::BeginChild("settingsScroll", {0, -(hud ? 88.f : 46.f) * s}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
    smoothScroll();

    auto group = [&](const char* title, auto&& belongs) {
        bool any = false;
        for (auto& set : m.settings())
            if (!isCore(set) && belongs(set) && set.shown()) any = true;
        if (!any) return;
        ImGuiID id = ImGui::GetID(title);
        bool& open = *ImGui::GetStateStorage()->GetBoolRef(id, true);
        ImVec2 hp = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        bool clicked = ImGui::InvisibleButton(title, {w, 34 * s});
        bool hov = ImGui::IsItemHovered();
        auto* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(hp, hp + ImVec2(w, 34 * s), theme::col(t.surfaceHover, hov ? 0.8f : 0.5f), 10 * s);
        dl->AddText(fonts::bold(), 16.f * s, hp + ImVec2(14 * s, 8 * s), theme::col(t.text), i18n::tr(title));
        ImVec2 ac{hp.x + w - 22 * s, hp.y + 17 * s};
        if (open) dl->AddTriangleFilled(ac + ImVec2(-5 * s, -3 * s), ac + ImVec2(5 * s, -3 * s), ac + ImVec2(0, 4 * s), theme::col(t.textDim));
        else dl->AddTriangleFilled(ac + ImVec2(-3 * s, -5 * s), ac + ImVec2(-3 * s, 5 * s), ac + ImVec2(4 * s, 0), theme::col(t.textDim));
        if (clicked) open = !open;
        if (open) {
            ImGui::Dummy({0, 4 * s});
            for (auto& set : m.settings())
                if (!isCore(set) && belongs(set)) widgets::setting(set);
        }
        ImGui::Dummy({0, 8 * s});
    };

    group("General", [&](const Setting& st) { return st.type != SettingType::Color && !isStyle(st); });
    m.drawSettings();
    group("Style", [&](const Setting& st) { return isStyle(st); });
    group("Colors", [&](const Setting& st) { return st.type == SettingType::Color; });
    ImGui::EndChild();

    {
        ImGui::Dummy({0, 4 * s});
        if (widgets::button("Reset all", {0, 0}, false))
            m.resetSettings([](const Setting& st) { return st.id != "x" && st.id != "y" && st.id != "key"; });
        if (hud) {
            ImGui::SameLine();
            if (widgets::button("Reset position", {0, 0}, false))
                m.resetSettings([](const Setting& st) { return st.id == "x" || st.id == "y" || st.id == "scale"; });
            if (widgets::button("Edit HUD", {ImGui::GetContentRegionAvail().x, 0}, true)) setEditingHud(true);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(2);
    if (selectedModule() && ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !widgets::capturingKey()) selectedModule() = nullptr;
}


static ImVec4 sectionColor(Category c) {
    switch (c) {
    case Category::Server: return {0.79f, 0.63f, 1.f, 1.f};
    case Category::Hud: return {0.55f, 0.91f, 0.69f, 1.f};
    case Category::Pvp: return {1.f, 0.56f, 0.71f, 1.f};
    case Category::Visual: return {0.49f, 0.78f, 1.f, 1.f};
    case Category::Comfort: return {1.f, 0.76f, 0.54f, 1.f};
    case Category::Performance: return {1.f, 0.89f, 0.49f, 1.f};
    default: return {0.76f, 0.7f, 0.82f, 1.f};
    }
}

static bool visible(const Module& m) {
    if (m.category() == Category::Client) return false;
    return true;
}

static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

static bool matches(const Module& m) {
    if (!visible(m)) return false;
    if (favoritesOnly() && !m.favorite()) return false;
    if (!searchText()[0]) return true;
    std::string q = lower(searchText());
    if (lower(i18n::tr(m.name().c_str())).find(q) != std::string::npos || lower(m.name()).find(q) != std::string::npos) return true;
    if (lower(i18n::tr(m.description().c_str())).find(q) != std::string::npos) return true;
    for (auto& tag : m.tags())
        if (lower(tag).find(q) != std::string::npos) return true;
    return false;
}

struct Section {
    std::string title;
    ImVec4 color;
    std::vector<Module*> items;
    bool fold = false;
    std::string id;
};

static std::map<std::string, bool>& folds() {
    static std::map<std::string, bool> open;
    return open;
}

void openAllGroups() {
    for (auto& m : modules::all()) folds()["more|" + std::string(categoryName(catOf(*m)))] = true;
    folds()["extras"] = true;
    folds()["servers"] = true;
}

static bool serverGroup(const Module& m) { return catOf(m) == Category::Server; }

// the short list first: what most players use. Everything else sits in folded groups below it.
static std::vector<Section> sections() {
    static const Category order[] = {Category::Pvp, Category::Hud, Category::Visual, Category::Comfort, Category::Performance};
    auto& t = theme::current();
    ImVec4 tone = theme::mix(t.textDim, t.text, 0.15f);

    std::vector<Module*> list;
    for (auto& m : modules::all())
        if (matches(*m)) list.push_back(m.get());

    std::map<std::string, int> subRank;
    for (auto* m : list) subRank.emplace(std::to_string((int)catOf(*m)) + "|" + m->sub(), (int)subRank.size());
    auto bySub = [&](Module* a, Module* b) {
        return subRank[std::to_string((int)catOf(*a)) + "|" + a->sub()] < subRank[std::to_string((int)catOf(*b)) + "|" + b->sub()];
    };

    std::vector<Section> out;
    Section fav{i18n::tr("Favorites"), {1.f, 0.84f, 0.45f, 1.f}, {}};
    for (auto* m : list)
        if (m->favorite()) fav.items.push_back(m);
    if (!fav.items.empty()) out.push_back(std::move(fav));

    bool searching = searchText()[0] != 0;
    // the PvP list leads with what decides a fight, in this order
    static const char* pvpFirst[] = {"Crystal Optimizer", "Block Hit", "Instant Hit", "Faster Inventory", "Item Use Delay Fix", "Insta Hurt Animation",
                                     "Hitbox", "Hurt Color", "Animations", "Reach Counter", "Combo Counter", "Hit Ping", "Target HUD", "Pot Counter",
                                     "Totem Counter", "Low Health Indicator", "Toggle Sprint", "Toggle Sneak", "Auto GG"};
    auto rank = [](const Module* m) {
        for (size_t i = 0; i < std::size(pvpFirst); i++)
            if (m->name() == pvpFirst[i]) return int(i);
        return int(std::size(pvpFirst));
    };
    auto add = [&](Section sec, bool curated = false) {
        if (curated) std::stable_sort(sec.items.begin(), sec.items.end(), [&](Module* a, Module* b) { return rank(a) < rank(b); });
        else std::stable_sort(sec.items.begin(), sec.items.end(), bySub);
        if (!sec.items.empty()) out.push_back(std::move(sec));
    };

    bool onServer = !rules::status().server.empty();
    static bool wasOnServer = false;
    if (onServer != wasOnServer) {
        folds()["servers"] = onServer;
        wasOnServer = onServer;
    }
    auto serverSection = [&] {
        Section sec{categoryName(Category::Server), tone, {}, !searching, "servers"};
        for (auto* m : list)
            if (!m->favorite() && serverGroup(*m)) sec.items.push_back(m);
        add(std::move(sec));
    };
    if (onServer) serverSection();

    std::vector<Category> cats(std::begin(order), std::end(order));
    if (searching) cats.push_back(Category::Fun);
    for (auto c : cats) {
        Section sec{categoryName(c), tone, {}};
        for (auto* m : list)
            if (!m->favorite() && !serverGroup(*m) && catOf(*m) == c && (searching || modules::tierOf(m->name()) <= 1)) sec.items.push_back(m);
        add(std::move(sec), c == Category::Pvp && !searching);
    }
    if (searching) {
        if (!onServer) serverSection();
        return out;
    }

    for (auto c : order) {
        Section sec{i18n::fmt("More {}", categoryName(c)), tone, {}, true, "more|" + std::string(categoryName(c))};
        for (auto* m : list)
            if (!m->favorite() && !serverGroup(*m) && catOf(*m) == c && modules::tierOf(m->name()) == 2) sec.items.push_back(m);
        add(std::move(sec));
    }
    Section extras{i18n::tr("Extras"), tone, {}, true, "extras"};
    for (auto* m : list)
        if (!m->favorite() && !serverGroup(*m) && modules::tierOf(m->name()) > 2) extras.items.push_back(m);
    std::stable_sort(extras.items.begin(), extras.items.end(), [](Module* a, Module* b) { return (int)catOf(*a) < (int)catOf(*b); });
    if (!extras.items.empty()) out.push_back(std::move(extras));
    if (!onServer) serverSection();
    return out;
}

static bool drawHeader(const Section& sec, size_t count) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    bool open = true;
    bool toggled = false;
    if (sec.fold) {
        auto& state = folds();
        bool& ref = state[sec.id];
        ImGui::PushID(sec.id.c_str());
        toggled = ImGui::InvisibleButton("fold", {w, 30 * s});
        bool hov = ImGui::IsItemHovered();
        ImGui::PopID();
        if (toggled) ref = !ref;
        open = ref;
        if (hov) dl->AddRectFilled(p, p + ImVec2(w, 30 * s), theme::col(t.surfaceHover, 0.35f), 9 * s);
    }
    std::string title = sec.title;
    for (auto& c : title)
        if ((unsigned char)c < 128) c = (char)std::toupper((unsigned char)c);
    ImVec4 c = sec.color;
    float x = p.x + 10 * s;
    if (sec.fold) {
        ImVec2 ac{p.x + 14 * s, p.y + 15 * s};
        ImU32 ic = theme::col(c, 0.85f);
        if (open) dl->AddTriangleFilled(ac + ImVec2(-4 * s, -2.5f * s), ac + ImVec2(4 * s, -2.5f * s), ac + ImVec2(0, 3 * s), ic);
        else dl->AddTriangleFilled(ac + ImVec2(-2.5f * s, -4 * s), ac + ImVec2(-2.5f * s, 4 * s), ac + ImVec2(3 * s, 0), ic);
        x = p.x + 28 * s;
    } else {
        dl->AddCircleFilled({p.x + 10 * s, p.y + 14 * s}, 3.2f * s, theme::col(c, 0.85f));
        x = p.x + 22 * s;
    }
    dl->AddText(fonts::bold(), 12.f * s, {x, p.y + 8 * s}, theme::col(c, 0.85f), title.c_str());
    ImVec2 ts = fonts::bold()->CalcTextSizeA(12.f * s, FLT_MAX, 0.f, title.c_str());
    char num[16];
    snprintf(num, sizeof(num), "%zu", count);
    ImVec2 ns = ImGui::CalcTextSize(num);
    dl->AddText({p.x + w - ns.x - 8 * s, p.y + 7 * s}, theme::col(t.textDim, 0.7f), num);
    dl->AddLine({x + 8 * s + ts.x, p.y + 14 * s}, {p.x + w - ns.x - 18 * s, p.y + 14 * s}, theme::col(c, 0.2f), 1.f);
    if (!sec.fold) ImGui::Dummy({w, 28 * s});
    else ImGui::SetCursorScreenPos(p + ImVec2(0, 32 * s));
    return open;
}

static void drawRow(Module& m, float width, ImVec4 color, bool isSelected, float fade) {
    auto& t = theme::current();
    float s = ui::scale();
    ImGui::PushID(m.name().c_str());
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 size{width, 38 * s};

    bool locked = !m.available() || m.rule() == RuleLevel::Block;
    float base = theme::fade();
    theme::setFade(base * fade);

    ImGui::SetNextItemAllowOverlap();
    bool clicked = ImGui::InvisibleButton("row", size);
    bool hovered = ImGui::IsItemHovered();
    m.hover = draw::approach(m.hover, hovered || isSelected ? 1.f : 0.f, 16.f * t.animSpeed);

    auto* dl = ImGui::GetWindowDrawList();
    float r = 10 * s;
    if (isSelected) dl->AddRectFilled(p, p + size, theme::col(t.surfaceHover, 0.9f), r);
    else if (m.hover > 0.01f) dl->AddRectFilled(p, p + size, theme::col(t.surfaceHover, 0.45f * m.hover), r);
    if (isSelected) dl->AddRectFilled({p.x, p.y + 9 * s}, {p.x + 3 * s, p.y + size.y - 9 * s}, theme::col(color, 0.95f), 2 * s);

    float cy = p.y + size.y * 0.5f;
    float dim = locked ? 0.42f : 1.f;
    const char* name = i18n::tr(m.name().c_str());
    dl->AddText(fonts::regular(), 16.5f * s, {p.x + 16 * s, cy - 10 * s}, theme::col(t.text, dim), name);

    float x = p.x + size.x - 10 * s;
    float tw = 40 * s;
    x -= tw;
    bool on = (m.userEnabled() && !locked) || m.alwaysOn();
    ImGui::SetCursorScreenPos({x, cy - 11 * s});
    bool state = on;
    if (widgets::toggle("t", state, !locked && !m.alwaysOn())) m.setEnabled(state);
    x -= 12 * s;

    if (locked) {
        float lw = 18 * s;
        x -= lw;
        glyph(dl, 13, {x + lw * 0.5f, cy}, 7 * s, theme::col(t.textDim, 0.6f));
        x -= 8 * s;
    } else if (m.keybind().i) {
        std::string k = widgets::keyName(m.keybind().i);
        ImVec2 ks = ImGui::CalcTextSize(k.c_str());
        float kw = ks.x + 14 * s;
        x -= kw;
        dl->AddRectFilled({x, cy - 10 * s}, {x + kw, cy + 10 * s}, theme::col(t.surfaceHover, 0.9f), 7 * s);
        dl->AddText({x + 7 * s, cy - ks.y * 0.5f}, theme::col(t.textDim), k.c_str());
        x -= 8 * s;
    }
    if (m.rule() == RuleLevel::Warn || m.risky()) {
        float ww = 18 * s;
        x -= ww;
        ImVec2 c{x + ww * 0.5f, cy};
        dl->AddTriangleFilled({c.x, c.y - 7 * s}, {c.x - 7 * s, c.y + 6 * s}, {c.x + 7 * s, c.y + 6 * s}, theme::col(t.warn, dim));
        dl->AddText(fonts::bold(), 11 * s, {c.x - 1.5f * s, c.y - 4 * s}, theme::col(t.bg), "!");
        if (ImGui::IsMouseHoveringRect(c - ImVec2(9 * s, 9 * s), c + ImVec2(9 * s, 9 * s)) && !m.ruleNote().empty()) {
            ImGui::BeginTooltip();
            ImGui::TextColored(t.warn, "%s", m.ruleNote().c_str());
            ImGui::EndTooltip();
        }
        x -= 6 * s;
    }

    float sw = 22 * s;
    x -= sw;
    ImVec2 sc{x + sw * 0.5f, cy};
    ImGui::SetCursorScreenPos({x, cy - sw * 0.5f});
    bool starClicked = ImGui::InvisibleButton("fav", {sw, sw});
    bool starHover = ImGui::IsItemHovered();
    if (m.favorite()) star(dl, sc, 7.5f * s, theme::col({1.f, 0.84f, 0.45f, 1.f}, dim), true);
    else if (hovered || starHover) star(dl, sc, 7.5f * s, theme::col(starHover ? t.text : t.textDim, 0.7f), false);
    if (starClicked) m.setFavorite(!m.favorite());

    if (hovered && locked) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(m.rule() == RuleLevel::Block ? m.ruleNote().c_str() : i18n::tr("Not available on this Minecraft version yet."));
        ImGui::EndTooltip();
    }
    if (clicked && !starClicked) selectedModule() = &m;

    theme::setFade(base);
    ImGui::SetCursorScreenPos(p + ImVec2(0, size.y + 2 * s));
    ImGui::PopID();
}

void drawModulesPage(ImVec2 origin, ImVec2 size) {
    auto& t = theme::current();
    float s = ui::scale();
    Module*& sel = selectedModule();
    if (sel) shown = sel;
    panelAnim = draw::approach(panelAnim, sel ? 1.f : 0.f, 14.f * t.animSpeed);

    float gap = 18 * s;
    float listW = std::min(size.x * 0.47f, 640 * s);
    panelOrigin = {origin.x + listW + gap, origin.y};
    panelSize = {size.x - listW - gap, size.y};

    size_t key = std::hash<std::string>{}(searchText()) * 31 + (favoritesOnly() ? 1 : 0);
    if (key != listKey) {
        listKey = key;
        listT = 0.f;
    }
    listT += ui::dt();

    ImGui::SetCursorScreenPos(origin);
    ImGui::BeginChild("modlist", {listW, size.y}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
    smoothScroll();
    auto secs = sections();
    if (secs.empty()) widgets::hint(favoritesOnly() ? "No favorites yet. Click the star next to a module." : "Nothing found.");
    int index = 0;
    for (auto& sec : secs) {
        bool open = drawHeader(sec, sec.items.size());
        if (!open) continue;
        for (auto* m : sec.items) {
            float e = draw::motion() ? draw::easeOutCubic(std::clamp((listT - index * 0.008f) / 0.25f, 0.f, 1.f)) : 1.f;
            drawRow(*m, ImGui::GetContentRegionAvail().x - 4 * s, sec.color, sel == m, e);
            index++;
        }
    }
    ImGui::Dummy({0, 12 * s});
    ImGui::EndChild();

    if (!sel && panelAnim < 0.02f) {
        auto* dl = ImGui::GetWindowDrawList();
        ImVec2 c = panelOrigin + panelSize * 0.5f;
        draw::heart(dl, c + ImVec2(0, -14 * s), 46 * s, theme::col(t.accent, 0.14f));
        const char* txt = i18n::tr("Pick a module to see its settings");
        ImVec2 ts = ImGui::CalcTextSize(txt);
        dl->AddText({c.x - ts.x * 0.5f, c.y + 34 * s}, theme::col(t.textDim, 0.7f), txt);
    }
}

void drawModulePanel() {
    if (panelAnim > 0.01f && shown) drawSettingsPanel(panelOrigin, panelSize, *shown, draw::easeOutCubic(panelAnim));
    else shown = nullptr;
}

}
