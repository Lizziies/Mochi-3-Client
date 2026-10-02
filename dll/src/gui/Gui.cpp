#include "Gui.hpp"
#include "GuiInternal.hpp"
#include "I18n.hpp"
#include "HudEditor.hpp"
#include "Theme.hpp"
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
static float sideAnim = 0.f;
static Page current = Page::Modules;
static Module* selected = nullptr;
static char search[64] = "";
static bool onlyFavorites = false;

char* searchText() { return search; }
bool& favoritesOnly() { return onlyFavorites; }
Page page() { return current; }
Module*& selectedModule() { return selected; }

void go(Page p) {
    if (p == Page::Hub) p = Page::Modules;
    if (p == current) return;
    current = p;
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

static bool textButton(const char* id, const char* label, bool active, ImVec2 at, float h, float& width) {
    auto& t = theme::current();
    float s = ui::scale();
    const char* text = i18n::tr(label);
    ImVec2 ts = fonts::regular()->CalcTextSizeA(14 * s, FLT_MAX, 0.f, text);
    width = ts.x + 18 * s;
    ImGui::SetCursorScreenPos(at);
    bool clicked = ImGui::InvisibleButton(id, {width, h});
    bool hov = ImGui::IsItemHovered();
    auto* dl = ImGui::GetWindowDrawList();
    if (hov || active) dl->AddRectFilled(at, at + ImVec2(width, h), theme::col(t.surfaceHover, active ? 0.9f : 0.6f), t.rounding * s);
    dl->AddText(fonts::regular(), 14 * s, {at.x + 9 * s, at.y + (h - ts.y) * 0.5f}, theme::col(active || hov ? t.text : t.textDim), text);
    if (active) dl->AddRectFilled({at.x + 9 * s, at.y + h - 3 * s}, {at.x + width - 9 * s, at.y + h - 1.5f * s}, theme::col(t.accent));
    return clicked;
}

static void header(ImVec2 pos, float w) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    float pad = 14 * s;
    float y = pos.y + 14 * s;
    std::string brand = build::name;
    for (auto& c : brand) c = (char)std::toupper((unsigned char)c);
    dl->AddText(fonts::bold(), 22 * s, {pos.x + pad, y + 2 * s}, theme::col(t.text), brand.c_str());
    float bw = fonts::bold()->CalcTextSizeA(22 * s, FLT_MAX, 0.f, brand.c_str()).x;
    dl->AddRectFilled({pos.x + pad + bw + 6 * s, y + 18 * s}, {pos.x + pad + bw + 11 * s, y + 23 * s}, theme::col(t.accent));

    float h = 28 * s;
    float x = pos.x + w - pad;
    float bwid = 0.f;
    struct Item {
        const char* id;
        const char* label;
        Page page;
    };
    const Item items[] = {{"hdr_settings", "Settings", Page::Settings}, {"hdr_cosmetics", "Cosmetics", Page::Cosmetics}};
    for (auto& it : items) {
        float width = fonts::regular()->CalcTextSizeA(14 * s, FLT_MAX, 0.f, i18n::tr(it.label)).x + 18 * s;
        x -= width;
        if (textButton(it.id, it.label, current == it.page, {x, y}, h, bwid)) go(current == it.page ? Page::Modules : it.page);
        x -= 4 * s;
    }
    float width = fonts::regular()->CalcTextSizeA(14 * s, FLT_MAX, 0.f, i18n::tr("Edit HUD")).x + 18 * s;
    x -= width;
    if (textButton("hdr_hud", "Edit HUD", false, {x, y}, h, bwid)) setEditingHud(true);
}

static void searchBar(ImVec2 at, float w) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    float h = 34 * s;
    float sw = 34 * s;
    float fw = w - sw - 6 * s;
    dl->AddRectFilled(at, at + ImVec2(fw, h), theme::col(t.surface), t.rounding * s);
    dl->AddRect(at, at + ImVec2(fw, h), theme::col(theme::border()), t.rounding * s, 0, 1.f);
    ImGui::SetCursorScreenPos(at + ImVec2(4 * s, 0));
    ImGui::SetNextItemWidth(fw - 8 * s);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {8 * s, (h - ImGui::GetFontSize()) * 0.5f});
    ImGui::InputTextWithHint("##search", i18n::tr("Search modules"), search, sizeof(search));
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImVec2 sp{at.x + w - sw, at.y};
    ImGui::SetCursorScreenPos(sp);
    bool clicked = ImGui::InvisibleButton("favs", {sw, h});
    bool hov = ImGui::IsItemHovered();
    ImVec4 bg = onlyFavorites ? theme::mix(t.surface, t.accent, 0.25f) : (hov ? t.surfaceHover : t.surface);
    dl->AddRectFilled(sp, sp + ImVec2(sw, h), theme::col(bg), t.rounding * s);
    dl->AddRect(sp, sp + ImVec2(sw, h), theme::col(onlyFavorites ? t.accent : theme::border()), t.rounding * s, 0, 1.f);
    star(dl, sp + ImVec2(sw, h) * 0.5f, 7 * s, theme::col(onlyFavorites ? ImVec4{1.f, 0.82f, 0.4f, 1.f} : t.textDim), onlyFavorites);
    if (hov) ImGui::SetTooltip("%s", i18n::tr("Favorites"));
    if (clicked) onlyFavorites = !onlyFavorites;
}

static void footer(ImVec2 at, float w) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    auto info = rules::status();
    std::string label = info.server.empty() ? std::string(i18n::tr("No server")) : info.server;
    if (info.blocked) label += "  ·  " + i18n::fmt("{} blocked", info.blocked);
    ImVec4 dot = info.server.empty() ? t.off : (info.blocked ? t.warn : t.ok);
    dl->AddLine(at, at + ImVec2(w, 0), theme::col(theme::border()), 1.f);
    float cy = at.y + 16 * s;
    dl->AddCircleFilled({at.x + 6 * s, cy}, 3.5f * s, theme::col(dot));
    dl->AddText(fonts::regular(), 13 * s, {at.x + 16 * s, cy - 7 * s}, theme::col(t.textDim), label.c_str());
    const char* hint = i18n::tr("Right Shift to close");
    ImVec2 hs = fonts::regular()->CalcTextSizeA(13 * s, FLT_MAX, 0.f, hint);
    dl->AddText(fonts::regular(), 13 * s, {at.x + w - hs.x, cy - 7 * s}, theme::col(t.textDim, 0.7f), hint);
}

static void openWindow(const char* id, ImVec2 pos, ImVec2 size, float fade) {
    auto& t = theme::current();
    float s = ui::scale();
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fade);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, t.rounding * s);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    ImGui::Begin(id, nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::GetWindowDrawList()->AddRect(pos, pos + size, theme::col(theme::border()), t.rounding * s, 0, 1.5f * s);
}

static void closeWindow() {
    ImGui::End();
    ImGui::PopStyleVar(3);
}

static ImVec2 listSize() {
    float s = ui::scale();
    auto ds = ImGui::GetIO().DisplaySize;
    return {std::clamp(410 * s, 300 * s, ds.x * 0.45f), ds.y - 36 * s};
}

static void drawList(float anim) {
    float s = ui::scale();
    float e = draw::easeOutCubic(std::clamp(anim, 0.f, 1.f));
    float fade = std::clamp(anim, 0.f, 1.f);
    ImVec2 size = listSize();
    ImVec2 pos{18 * s - (1.f - e) * 26 * s, 18 * s};
    theme::setFade(fade);
    openWindow("##mochi", pos, size, fade);
    header(pos, size.x);
    float pad = 14 * s;
    searchBar({pos.x + pad, pos.y + 56 * s}, size.x - pad * 2);
    float top = pos.y + 56 * s + 34 * s + 10 * s;
    float foot = 34 * s;
    drawModulesPage({pos.x + 8 * s, top}, {size.x - 12 * s, pos.y + size.y - foot - top});
    footer({pos.x + pad, pos.y + size.y - foot}, size.x - pad * 2);
    closeWindow();
    theme::setFade(1.f);
}

static void drawSide(Page page, float anim) {
    auto& t = theme::current();
    float s = ui::scale();
    auto ds = ImGui::GetIO().DisplaySize;
    float e = draw::easeOutCubic(std::clamp(anim, 0.f, 1.f));
    float fade = std::clamp(anim, 0.f, 1.f);
    ImVec2 list = listSize();
    float x = 18 * s + list.x + 12 * s;
    ImVec2 size{std::min(ds.x - x - 18 * s, 980 * s), list.y};
    ImVec2 pos{x - (1.f - e) * 20 * s, 18 * s};
    theme::setFade(fade);
    openWindow("##mochi_side", pos, size, fade);
    auto* dl = ImGui::GetWindowDrawList();
    float pad = 18 * s;
    const char* title = page == Page::Cosmetics ? "Cosmetics" : "Settings";
    dl->AddText(fonts::bold(), 20 * s, pos + ImVec2(pad, 16 * s), theme::col(t.text), i18n::tr(title));
    ImVec2 cp{pos.x + size.x - pad - 26 * s, pos.y + 14 * s};
    ImGui::SetCursorScreenPos(cp);
    bool close = ImGui::InvisibleButton("sideclose", {26 * s, 26 * s});
    bool hov = ImGui::IsItemHovered();
    if (hov) dl->AddRectFilled(cp, cp + ImVec2(26 * s, 26 * s), theme::col(t.surfaceHover), t.rounding * s);
    ImVec2 c = cp + ImVec2(13 * s, 13 * s);
    ImU32 xc = theme::col(hov ? t.text : t.textDim);
    dl->AddLine(c + ImVec2(-4.5f * s, -4.5f * s), c + ImVec2(4.5f * s, 4.5f * s), xc, 1.6f * s);
    dl->AddLine(c + ImVec2(4.5f * s, -4.5f * s), c + ImVec2(-4.5f * s, 4.5f * s), xc, 1.6f * s);
    ImVec2 origin{pos.x + pad, pos.y + 56 * s};
    ImVec2 area{size.x - pad * 2, size.y - 56 * s - pad};
    if (page == Page::Cosmetics) drawCosmeticsPage(origin, area);
    else drawSettingsPage(origin, area);
    closeWindow();
    theme::setFade(1.f);
    if (close) go(Page::Modules);
}

static void backdrop(float strength) {
    auto ds = ImGui::GetIO().DisplaySize;
    ImGui::GetBackgroundDrawList()->AddRectFilled({0, 0}, ds, IM_COL32(0, 0, 0, int(70 * strength)));
}

static void drawMenu() {
    auto& t = theme::current();
    openAnim = draw::approach(openAnim, isOpen ? 1.f : 0.f, 14.f * t.animSpeed);
    sideAnim = draw::approach(sideAnim, isOpen && current != Page::Modules ? 1.f : 0.f, 14.f * t.animSpeed);
    if (openAnim < 0.01f) return;

    static Page side = Page::Settings;
    if (current != Page::Modules) side = current;
    backdrop(std::clamp(openAnim, 0.f, 1.f));
    drawList(openAnim);
    if (sideAnim > 0.01f) drawSide(side, sideAnim);

    bool typing = ImGui::GetIO().WantTextInput;
    if (isOpen && ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !widgets::capturingKey() && !typing) {
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
