#include "Gui.hpp"
#include "GuiInternal.hpp"
#include "I18n.hpp"
#include "HudEditor.hpp"
#include "Icons.hpp"
#include "Theme.hpp"
#include "Tiles.hpp"
#include "Widgets.hpp"
#include "core/Build.hpp"
#include "core/Config.hpp"
#include "hook/Input.hpp"
#include "modules/Manager.hpp"
#include "modules/client/ClientSettings.hpp"
#include "modules/Tiers.hpp"
#include "render/Draw.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"
#include "server/Rules.hpp"

#include <imgui.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <string>

namespace gui {

static std::atomic<bool> isOpen{false};
static std::atomic<bool> hudEdit{false};
static std::atomic<bool> keyboardClaim{false};
static bool keyboardClaimNext = false;
static float openAnim = 0.f;
static Page current = Page::Modules;
static Module* selected = nullptr;
static char search[64] = "";
static bool focusSearch = false;
static bool onlyFavorites = false;
static float contentT = 1.f;

void restartContentAnim() { contentT = 0.f; }

char* searchText() { return search; }
bool& favoritesOnly() { return onlyFavorites; }
Page page() { return current; }
Module*& selectedModule() { return selected; }

void go(Page p) {
    if (p == Page::Hub) p = Page::Modules;
    if (p == current) return;
    current = p;
    closePanel();
    contentT = 0.f;
    focusSearch = false;
    if (p == Page::Cosmetics) reloadCosmetics();
}

void beginFrame() {
    keyboardClaim = keyboardClaimNext;
    keyboardClaimNext = false;
}

bool open() { return isOpen; }

void setOpen(bool on) {
    if (on && !isOpen) input::releaseHeld();
    isOpen = on;
    if (on) {
        hudEdit = false;
    } else {
        config::saveIfDirty();
    }
}

void toggle() {
    if (hudEdit) {
        hudEdit = false;
        setOpen(true);
        return;
    }
    if (isOpen) {
        setOpen(false);
        return;
    }
    if (current != Page::Modules) go(Page::Modules);
    search[0] = 0;
    setOpen(true);
}

void showModule(Module* m) {
    current = Page::Modules;
    setOpen(true);
    closePanel();
    selected = m;
}

bool editingHud() { return hudEdit; }

void setEditingHud(bool on) {
    if (on && !hudEdit) input::releaseHeld();
    hudEdit = on;
    if (on) isOpen = false;
}

float menuBlurPx() {
    auto* cs = modules::get<ClientSettings>();
    float strength = cs ? cs->menuBlur() : 0.f;
    return strength * 22.f * ui::scale() * std::clamp(openAnim, 0.f, 1.f);
}

bool wantsInput() { return isOpen || hudEdit || keyboardClaim; }
bool wantsCursor() { return isOpen || hudEdit; }
bool capturesKeyboard() { return isOpen || widgets::capturingKey() || keyboardClaim || ImGui::GetIO().WantTextInput; }
void claimKeyboard() { keyboardClaimNext = true; }

static bool roundButton(const char* id, ImVec2 at, float size) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    ImGui::SetCursorScreenPos(at);
    bool clicked = ImGui::InvisibleButton(id, {size, size});
    bool hov = ImGui::IsItemHovered();
    dl->AddRectFilled(at, at + ImVec2(size, size), theme::col(hov ? t.surfaceHover : t.surface), 8 * s);
    ImVec2 c = at + ImVec2(size, size) * 0.5f;
    ImU32 col = theme::col(hov ? t.text : t.textDim);
    float r = 5 * s;
    dl->AddLine(c + ImVec2(-r, -r), c + ImVec2(r, r), col, 1.8f * s);
    dl->AddLine(c + ImVec2(r, -r), c + ImVec2(-r, r), col, 1.8f * s);
    return clicked;
}

static bool chip(const char* id, const char* label, bool active, ImVec2 at, float h) {
    auto& t = theme::current();
    float s = ui::scale();
    const char* text = i18n::tr(label);
    ImVec2 ts = ImGui::CalcTextSize(text);
    ImVec2 size{ts.x + 44 * s, h};
    ImGui::SetCursorScreenPos(at);
    bool clicked = ImGui::InvisibleButton(id, size);
    bool hov = ImGui::IsItemHovered();
    auto* dl = ImGui::GetWindowDrawList();
    ImVec4 bg = active ? theme::mix(t.surface, t.accent, 0.2f) : (hov ? t.surfaceHover : t.surface);
    dl->AddRectFilled(at, at + size, theme::col(bg), 8 * s);
    if (active) dl->AddRect(at, at + size, theme::col(t.accent, 0.7f), 8 * s, 0, 1.2f * s);
    star(dl, {at.x + 16 * s, at.y + h * 0.5f}, 6.5f * s, theme::col(active ? ImVec4{1.f, 0.82f, 0.4f, 1.f} : t.textDim), active);
    dl->AddText({at.x + 30 * s, at.y + (h - ts.y) * 0.5f}, theme::col(active ? t.text : t.textDim), text);
    return clicked;
}

static void topBar(float left, float right, float y) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    float h = 36 * s;

    if (current == Page::Modules) {
        float w = std::min(380 * s, (right - left) * 0.45f);
        ImVec2 p{left, y};
        dl->AddRectFilled(p, p + ImVec2(w, h), theme::col(t.surface), 8 * s);
        icon(dl, Icon::Zoom, p + ImVec2(18 * s, h * 0.5f), 6 * s, theme::col(t.textDim));
        ImGui::SetCursorScreenPos(p + ImVec2(32 * s, 0));
        ImGui::SetNextItemWidth(w - 40 * s);
        if (focusSearch) {
            ImGui::SetKeyboardFocusHere();
            focusSearch = false;
        }
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {6 * s, (h - ImGui::GetFontSize()) * 0.5f});
        ImGui::InputTextWithHint("##search", i18n::tr("Search modules"), search, sizeof(search));
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        if (chip("favs", "Favorites", onlyFavorites, {left + w + 10 * s, y}, h)) onlyFavorites = !onlyFavorites;
    } else {
        const char* title = current == Page::Cosmetics ? "Cosmetics" : "Settings";
        dl->AddText(fonts::bold(), 22.f * s, {left, y + (h - 22.f * s) * 0.5f}, theme::col(t.text), i18n::tr(title));
    }

    if (roundButton("close", {right - h, y}, h)) setOpen(false);
    drawServerChip({right - h - 300 * s, y}, {right - h - 10 * s, y + h}, right - h - 10 * s);
}

static bool navItem(const char* id, Icon kind, const char* label, int count, bool active, ImVec2 p, float w) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    float h = 36 * s;
    ImGui::SetCursorScreenPos(p);
    bool clicked = ImGui::InvisibleButton(id, {w, h});
    bool hov = ImGui::IsItemHovered();
    if (active) {
        dl->AddRectFilled(p, p + ImVec2(w, h), theme::col(theme::mix(t.surface, t.accent, 0.16f)), 8 * s);
        dl->AddRectFilled({p.x, p.y + 9 * s}, {p.x + 3 * s, p.y + h - 9 * s}, theme::col(t.accent), 2 * s);
    } else if (hov) {
        dl->AddRectFilled(p, p + ImVec2(w, h), theme::col(t.surfaceHover, 0.6f), 8 * s);
    }
    ImU32 col = theme::col(active ? t.text : (hov ? t.text : t.textDim));
    icon(dl, kind, {p.x + 22 * s, p.y + h * 0.5f}, 7.5f * s, theme::col(active ? t.accent : t.textDim));
    dl->AddText(fonts::regular(), 15 * s, {p.x + 40 * s, p.y + (h - 15 * s) * 0.5f}, col, label);
    if (count > 0) {
        std::string n = std::to_string(count);
        ImVec2 ns = ImGui::CalcTextSize(n.c_str());
        ImVec2 b{p.x + w - ns.x - 22 * s, p.y + (h - 18 * s) * 0.5f};
        dl->AddRectFilled(b, b + ImVec2(ns.x + 12 * s, 18 * s), theme::col(active ? t.accent : t.surfaceHover, active ? 0.9f : 1.f), 9 * s);
        dl->AddText({b.x + 6 * s, b.y + (18 * s - ns.y) * 0.5f}, theme::col(active ? t.bg : t.textDim), n.c_str());
    }
    return clicked;
}

static void sidebar(ImVec2 pos, ImVec2 size) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, pos + size, theme::col(t.surface, 0.55f), 10 * s, ImDrawFlags_RoundCornersLeft);
    dl->AddLine({pos.x + size.x, pos.y}, {pos.x + size.x, pos.y + size.y}, theme::col(t.surfaceHover), 1.f);

    float pad = 12 * s;
    float x = pos.x + pad;
    float w = size.x - pad * 2;
    float y = pos.y + 18 * s;
    dl->AddRectFilled({x + 4 * s, y + 3 * s}, {x + 10 * s, y + 21 * s}, theme::col(t.accent), 2 * s);
    std::string brand = build::name;
    for (auto& c : brand) c = (char)std::toupper((unsigned char)c);
    dl->AddText(fonts::bold(), 20 * s, {x + 18 * s, y}, theme::col(t.text), brand.c_str());
    dl->AddText(fonts::regular(), 11.5f * s, {x + 18 * s, y + 23 * s}, theme::col(t.textDim), i18n::tr("PvP Client"));
    y += 60 * s;

    int counts[sectionCount] = {};
    int total = 0;
    for (auto& tile : tiles()) {
        counts[int(tile.section)] += tile.enabled();
        total += tile.enabled();
    }
    bool modules = current == Page::Modules && !search[0] && !onlyFavorites;
    if (navItem("nav_all", Icon::Grid, i18n::tr("All"), total, modules && sectionIndex() < 0, {x, y}, w)) {
        go(Page::Modules);
        sectionIndex() = -1;
        search[0] = 0;
        onlyFavorites = false;
    }
    y += 40 * s;
    for (int i = 0; i < sectionCount; i++) {
        ImGui::PushID(i);
        if (navItem("nav", sectionIcon(Section(i)), sectionName(Section(i)), counts[i], modules && sectionIndex() == i, {x, y}, w)) {
            go(Page::Modules);
            sectionIndex() = i;
            search[0] = 0;
            onlyFavorites = false;
            closePanel();
        }
        ImGui::PopID();
        y += 40 * s;
    }

    float bottom = pos.y + size.y - pad;
    float by = bottom - 38 * s;
    ImGui::SetCursorScreenPos({x, by});
    if (widgets::button("Edit HUD", {w, 36 * s}, true)) setEditingHud(true);
    by -= 46 * s;
    if (navItem("nav_settings", Icon::Gear, i18n::tr("Settings"), 0, current == Page::Settings, {x, by}, w)) go(Page::Settings);
    by -= 40 * s;
    if (navItem("nav_cosmetics", Icon::Sparkle, i18n::tr("Cosmetics"), 0, current == Page::Cosmetics, {x, by}, w)) go(Page::Cosmetics);
}

static void backdrop(float strength) {
    auto ds = ImGui::GetIO().DisplaySize;
    auto* bg = ImGui::GetBackgroundDrawList();
    bg->AddRectFilled({0, 0}, ds, IM_COL32(4, 5, 8, int(140 * strength)));
}

static void drawFrame(float anim) {
    auto& t = theme::current();
    float s = ui::scale();
    auto ds = ImGui::GetIO().DisplaySize;
    float e = draw::easeOutCubic(std::clamp(anim, 0.f, 1.f));
    ImVec2 size{std::min(1180 * s, ds.x * 0.9f), std::min(700 * s, ds.y * 0.86f)};
    ImVec2 pos = (ds - size) * 0.5f + ImVec2(0, (1.f - e) * 16 * s);
    float fade = std::clamp(anim, 0.f, 1.f);
    theme::setFade(fade);

    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fade);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10 * s);
    ImGui::Begin("##mochi", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRect(pos, pos + size, theme::col(t.surfaceHover), 10 * s, 0, 1.f);

    float sideW = 200 * s;
    sidebar(pos, {sideW, size.y});

    float pad = 20 * s;
    float left = pos.x + sideW + pad;
    float right = pos.x + size.x - pad;
    float top = pos.y + 16 * s;
    topBar(left, right, top);

    contentT = draw::motion() ? std::min(1.f, contentT + ui::dt() / 0.25f) : 1.f;
    float ce = draw::easeOutCubic(contentT);
    theme::setFade(fade * ce);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fade * ce);
    ImVec2 origin{left, top + 36 * s + 18 * s + (1.f - ce) * 10 * s};
    ImVec2 area{right - left, pos.y + size.y - 18 * s - origin.y};
    switch (current) {
    case Page::Cosmetics: drawCosmeticsPage(origin, area); break;
    case Page::Settings: drawSettingsPage(origin, area); break;
    default: drawModulesPage(origin, area); break;
    }
    ImGui::PopStyleVar();
    theme::setFade(fade);
    ImGui::End();
    ImGui::PopStyleVar(2);
    if (current == Page::Modules) drawModulePanel();
    theme::setFade(1.f);
}

static void drawMenu() {
    auto& t = theme::current();
    openAnim = draw::approach(openAnim, isOpen ? 1.f : 0.f, 14.f * t.animSpeed);
    if (openAnim < 0.01f) return;

    bool panelBefore = panelOpen();
    backdrop(std::clamp(openAnim, 0.f, 1.f));
    drawFrame(openAnim);
    theme::setFade(1.f);

    if (isOpen && ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !panelBefore && !widgets::capturingKey()) {
        if (current != Page::Modules) go(Page::Modules);
        else setOpen(false);
    }
}

void draw() {
    pollDevCommands();
    if (hudEdit) {
        hudeditor::draw();
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            hudEdit = false;
            setOpen(true);
        }
    }
    drawMenu();
}

}
