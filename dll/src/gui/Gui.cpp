#include "Gui.hpp"
#include "GuiInternal.hpp"
#include "I18n.hpp"
#include "HudEditor.hpp"
#include "Theme.hpp"
#include "Widgets.hpp"
#include "core/Config.hpp"
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
#include <cmath>

namespace gui {

static std::atomic<bool> isOpen{false};
static std::atomic<bool> hudEdit{false};
static std::atomic<bool> keyboardClaim{false};
static bool keyboardClaimNext = false;
static float openAnim = 0.f;
static float hubT = 0.f;
static float frameT = 0.f;
static Page current = Page::Hub;
static Module* selected = nullptr;
static char search[64] = "";
static bool focusSearch = false;
static bool showMore = false;
static bool onlyFavorites = false;
static float contentT = 1.f;

void restartContentAnim() { contentT = 0.f; }

char* searchText() { return search; }
bool& showMoreModules() { return showMore; }
bool& favoritesOnly() { return onlyFavorites; }
Page page() { return current; }
Module*& selectedModule() { return selected; }

void go(Page p) {
    if (p == current) return;
    current = p;
    selected = nullptr;
    contentT = 0.f;
    focusSearch = p == Page::Modules;
    if (p == Page::Cosmetics) reloadCosmetics();
    if (p == Page::Hub) search[0] = 0;
}

void beginFrame() {
    keyboardClaim = keyboardClaimNext;
    keyboardClaimNext = false;
}

bool open() { return isOpen; }

void setOpen(bool on) {
    isOpen = on;
    if (on) {
        hudEdit = false;
        focusSearch = current == Page::Hub || current == Page::Modules;
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
    current = Page::Hub;
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

static bool circleButton(const char* id, ImVec2 at, float size, bool accent, int kind) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    ImGui::SetCursorScreenPos(at);
    bool clicked = ImGui::InvisibleButton(id, {size, size});
    bool hov = ImGui::IsItemHovered();
    dl->AddRectFilled(at, at + ImVec2(size, size), theme::col(hov && accent ? t.accent : t.surface, hov && accent ? 0.9f : 1.f), size * 0.5f);
    ImVec2 c = at + ImVec2(size, size) * 0.5f;
    ImU32 col = theme::col(hov && accent ? t.bg : t.textDim);
    float r = 5 * s;
    if (kind == 0) {
        dl->AddLine(c + ImVec2(-r, -r), c + ImVec2(r, r), col, 2.f * s);
        dl->AddLine(c + ImVec2(r, -r), c + ImVec2(-r, r), col, 2.f * s);
    } else {
        dl->AddLine(c + ImVec2(r * 0.9f, 0), c + ImVec2(-r, 0), col, 2.f * s);
        dl->AddLine(c + ImVec2(-r, 0), c + ImVec2(0, -r), col, 2.f * s);
        dl->AddLine(c + ImVec2(-r, 0), c + ImVec2(0, r), col, 2.f * s);
    }
    return clicked;
}

static bool chip(const char* id, const char* label, bool active, ImVec4 tint, ImVec2 at, bool withStar, float& width) {
    auto& t = theme::current();
    float s = ui::scale();
    const char* text = i18n::tr(label);
    ImVec2 ts = ImGui::CalcTextSize(text);
    float starW = withStar ? 22 * s : 0.f;
    ImVec2 size{ts.x + 28 * s + starW, 34 * s};
    width = size.x;
    ImGui::SetCursorScreenPos(at);
    bool clicked = ImGui::InvisibleButton(id, size);
    bool hov = ImGui::IsItemHovered();
    auto* dl = ImGui::GetWindowDrawList();
    ImVec4 bg = active ? theme::mix(t.surface, tint, 0.25f) : theme::mix(t.surface, t.surfaceHover, hov ? 1.f : 0.f);
    dl->AddRectFilled(at, at + size, theme::col(bg), size.y * 0.5f);
    if (active) dl->AddRect(at, at + size, theme::col(tint, 0.6f), size.y * 0.5f, 0, 1.2f * s);
    float x = at.x + 14 * s;
    if (withStar) {
        star(dl, {x + 6 * s, at.y + size.y * 0.5f}, 6.5f * s, theme::col(active ? tint : t.textDim), active);
        x += starW;
    }
    dl->AddText({x, at.y + (size.y - ts.y) * 0.5f}, theme::col(active ? t.text : t.textDim), text);
    return clicked;
}

static void drawBar(float left, float right, float y) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    float h = 36 * s;

    if (circleButton("back", {left, y}, 36 * s, false, 1)) go(Page::Hub);
    float x = left + 50 * s;
    if (current == Page::Modules) {
        ImGui::SetCursorScreenPos({x, y});
        ImGui::SetNextItemWidth(300 * s);
        if (focusSearch) {
            ImGui::SetKeyboardFocusHere();
            focusSearch = false;
        }
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 99.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {16 * s, (h - ImGui::GetFontSize()) * 0.5f});
        ImGui::InputTextWithHint("##search", i18n::tr("Search all modules …"), search, sizeof(search));
        ImGui::PopStyleVar(2);
        float w = 0.f;
        x += 312 * s;
        if (chip("favs", "Favorites", onlyFavorites, {1.f, 0.84f, 0.45f, 1.f}, {x, y + 1 * s}, true, w)) onlyFavorites = !onlyFavorites;
        x += w + 8 * s;
        if (chip("more", "More modules", showMore, t.accent, {x, y + 1 * s}, false, w)) showMore = !showMore;
    } else {
        const char* title = current == Page::Cosmetics ? "Cosmetics" : "Settings";
        dl->AddText(fonts::bold(), 24.f * s, {x, y + (h - 24.f * s) * 0.5f}, theme::col(t.text), i18n::tr(title));
    }

    if (circleButton("close", {right - 36 * s, y}, 36 * s, true, 0)) setOpen(false);
    float bx = right - 36 * s - 130 * s;
    ImGui::SetCursorScreenPos({bx, y + 1 * s});
    if (widgets::button("Edit HUD", {120 * s, 34 * s}, true)) setEditingHud(true);
    drawServerChip({bx - 220 * s, y}, {bx, y + h}, bx - 12 * s);
}

static void backdrop(float strength) {
    auto ds = ImGui::GetIO().DisplaySize;
    auto* bg = ImGui::GetBackgroundDrawList();
    bg->AddRectFilledMultiColor({0, 0}, ds, IM_COL32(8, 3, 12, int(150 * strength)), IM_COL32(8, 3, 12, int(150 * strength)),
                                IM_COL32(28, 8, 36, int(215 * strength)), IM_COL32(28, 8, 36, int(215 * strength)));
}

static void beginWindow(const char* id, ImVec2 pos, ImVec2 size, float alpha) {
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    ImGui::Begin(id, nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    draw::glow(dl, pos, pos + size, t.rounding * s, theme::col(t.accent, 0.22f), 22 * s);
    dl->AddRect(pos, pos + size, theme::col(t.accent, 0.25f), t.rounding * s, 0, 1.2f);
}

static void endWindow() {
    ImGui::End();
    ImGui::PopStyleVar();
}

struct HubCard {
    const char* title;
    Page target;
    int glyphKind;
    ImVec4 color;
};

static void drawHub(float anim) {
    auto& t = theme::current();
    float s = ui::scale();
    auto ds = ImGui::GetIO().DisplaySize;
    float e = draw::easeOutBack(std::clamp(anim, 0.f, 1.f));
    bool anyFav = std::any_of(modules::all().begin(), modules::all().end(), [](auto& m) { return m->favorite() && m->category() != Category::Client; });
    ImVec2 size{std::min(720 * s, ds.x * 0.8f), (anyFav ? 330.f : 252.f) * s};
    ImVec2 pos = (ds - size) * 0.5f + ImVec2(0, (1.f - e) * 22 * s);
    float fade = std::clamp(anim, 0.f, 1.f);
    theme::setFade(fade);
    beginWindow("##mochi_hub", pos, size, fade);
    auto* dl = ImGui::GetWindowDrawList();

    float pad = 20 * s;
    float topY = pos.y + 16 * s;
    drawLogo(dl, {pos.x + pad, topY - 2 * s});

    float rx = pos.x + size.x - pad;
    float bh = 34 * s;
    float bw = std::max(92.f * s, ImGui::CalcTextSize(i18n::tr("Edit HUD")).x + 34 * s);
    ImGui::SetCursorScreenPos({rx - bw, topY});
    if (widgets::button("Edit HUD", {bw, bh}, true)) setEditingHud(true);
    if (circleButton("hubclose", {rx - bw - 10 * s - bh, topY}, bh, true, 0)) setOpen(false);

    float sx = pos.x + pad + 150 * s;
    ImGui::SetCursorScreenPos({sx, topY});
    ImGui::SetNextItemWidth(rx - bw - 10 * s - bh - 10 * s - sx);
    if (focusSearch) {
        ImGui::SetKeyboardFocusHere();
        focusSearch = false;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 99.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {14 * s, (bh - ImGui::GetFontSize()) * 0.5f});
    bool typed = ImGui::InputTextWithHint("##hubsearch", i18n::tr("Type to search modules …"), search, sizeof(search));
    bool enter = ImGui::IsItemDeactivated() && ImGui::IsKeyPressed(ImGuiKey_Enter);
    ImGui::PopStyleVar(2);
    if ((typed && search[0]) || enter) {
        char keep[64];
        snprintf(keep, sizeof(keep), "%s", search);
        go(Page::Modules);
        snprintf(search, sizeof(search), "%s", keep);
    }

    auto& mods = modules::all();
    int ready = 0, total = 0, favs = 0;
    for (auto& m : mods) {
        if (m->category() == Category::Client) continue;
        total++;
        if (m->available()) ready++;
        if (m->favorite()) favs++;
    }
    const HubCard cards[] = {
        {"Modules", Page::Modules, -1, {1.f, 0.56f, 0.71f, 1.f}},
        {"Favorites", Page::Modules, 100, {1.f, 0.84f, 0.45f, 1.f}},
        {"Cosmetics", Page::Cosmetics, 6, {0.79f, 0.63f, 1.f, 1.f}},
        {"Settings", Page::Settings, 12, {0.49f, 0.78f, 1.f, 1.f}},
    };
    std::string subs[4] = {i18n::fmt("{} · {} ready", total, ready),
                           favs ? i18n::fmt("{} starred", favs) : std::string(i18n::tr("Pin modules")),
                           i18n::tr("Free"), i18n::tr("Language, tag, look")};

    float gap = 10 * s;
    float cw = (size.x - pad * 2 - gap * 3) / 4.f;
    float ch = 124 * s;
    float cy = pos.y + 64 * s;
    static float hover[4] = {};
    for (int i = 0; i < 4; i++) {
        float stagger = draw::motion() ? draw::easeOutCubic(std::clamp((anim - i * 0.08f) / 0.5f, 0.f, 1.f)) : 1.f;
        ImVec2 p{pos.x + pad + i * (cw + gap), cy + (1.f - stagger) * 14 * s};
        ImGui::SetCursorScreenPos(p);
        ImGui::PushID(i);
        bool clicked = ImGui::InvisibleButton("card", {cw, ch});
        bool hov = ImGui::IsItemHovered();
        ImGui::PopID();
        hover[i] = draw::approach(hover[i], hov ? 1.f : 0.f, 16.f * t.animSpeed);
        ImVec2 a = p - ImVec2(0, hover[i] * 3 * s), b = a + ImVec2(cw, ch);
        const HubCard& c = cards[i];
        theme::setFade(fade * stagger);
        float r = 16 * s;
        if (hover[i] > 0.01f) draw::glow(dl, a, b, r, theme::col(c.color, 0.26f * hover[i]), 10 * s);
        draw::gradientRect(dl, a, b, theme::col(theme::mix(t.surface, c.color, 0.2f + 0.08f * hover[i])), theme::col(theme::mix(t.surface, c.color, 0.06f)), r);
        dl->AddRect(a, b, theme::col(c.color, 0.22f + 0.35f * hover[i]), r, 0, 1.2f * s);
        ImVec2 gc{(a.x + b.x) * 0.5f, a.y + 38 * s};
        ImU32 gcol = theme::col(c.color, 0.8f + 0.2f * hover[i]);
        if (c.glyphKind == 100) star(dl, gc, 17 * s, gcol, true);
        else glyph(dl, c.glyphKind, gc, 17 * s, gcol);
        draw::textCentered(dl, fonts::bold(), 17.f * s, {gc.x, a.y + 76 * s}, theme::col(t.text), i18n::tr(c.title));
        draw::textCentered(dl, fonts::regular(), 12.5f * s, {gc.x, a.y + 98 * s}, theme::col(t.textDim), subs[i].c_str());
        if (clicked) {
            if (i == 1) onlyFavorites = true;
            else if (i == 0) onlyFavorites = false;
            go(c.target);
        }
    }
    theme::setFade(fade);

    float y = cy + ch + 14 * s;
    if (anyFav) {
        float x = pos.x + pad;
        int rows = 0;
        int shownCount = 0;
        for (auto& mp : mods) {
            Module& m = *mp;
            if (!m.favorite() || m.category() == Category::Client || shownCount >= 10) continue;
            const char* name = i18n::tr(m.name().c_str());
            ImVec2 ts = ImGui::CalcTextSize(name);
            float w = ts.x + 74 * s;
            if (x + w > pos.x + size.x - pad) {
                x = pos.x + pad;
                y += 38 * s;
                if (++rows >= 2) break;
            }
            dl->AddRectFilled({x, y}, {x + w, y + 32 * s}, theme::col(t.surface), 16 * s);
            dl->AddText({x + 13 * s, y + (32 * s - ts.y) * 0.5f}, theme::col(t.text), name);
            ImGui::SetCursorScreenPos({x + w - 50 * s, y + 5 * s});
            ImGui::PushID(m.name().c_str());
            bool locked = !m.available() || m.rule() == RuleLevel::Block;
            bool on = m.userEnabled() && !locked;
            if (widgets::toggle("q", on, !locked) && !locked) m.setEnabled(on);
            ImGui::PopID();
            x += w + 8 * s;
            shownCount++;
        }
    }
    const char* foot = i18n::tr("Right Shift or Esc: close");
    ImVec2 fs = ImGui::CalcTextSize(foot);
    dl->AddText({pos.x + (size.x - fs.x) * 0.5f, pos.y + size.y - 28 * s}, theme::col(t.textDim, 0.7f), foot);
    endWindow();
}

static void drawFrame(float anim) {
    auto& t = theme::current();
    float s = ui::scale();
    auto ds = ImGui::GetIO().DisplaySize;
    float e = draw::easeOutBack(std::clamp(anim, 0.f, 1.f));
    ImVec2 size{std::min(1360 * s, ds.x * 0.86f), std::min(820 * s, ds.y * 0.84f)};
    ImVec2 pos = (ds - size) * 0.5f + ImVec2(0, (1.f - e) * 24 * s);
    float fade = std::clamp(anim, 0.f, 1.f);
    theme::setFade(fade);
    beginWindow("##mochi", pos, size, fade);

    float pad = 24 * s;
    drawBar(pos.x + pad, pos.x + size.x - pad, pos.y + 18 * s);
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddLine({pos.x + pad, pos.y + 66 * s}, {pos.x + size.x - pad, pos.y + 66 * s}, theme::col(t.surfaceHover, 0.7f), 1.f);

    contentT = draw::motion() ? std::min(1.f, contentT + ui::dt() / 0.3f) : 1.f;
    float ce = draw::easeOutCubic(contentT);
    theme::setFade(fade * ce);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fade * ce);
    ImVec2 origin{pos.x + pad, pos.y + 80 * s + (1.f - ce) * 16 * s};
    ImVec2 area{size.x - pad * 2, size.y - 80 * s - 22 * s};
    switch (current) {
    case Page::Modules: drawModulesPage(origin, area); break;
    case Page::Cosmetics: drawCosmeticsPage(origin, area); break;
    case Page::Settings: drawSettingsPage(origin, area); break;
    default: break;
    }
    ImGui::PopStyleVar();
    theme::setFade(fade);
    endWindow();
    if (current == Page::Modules) drawModulePanel();
    theme::setFade(1.f);
}

static void drawMenu() {
    auto& t = theme::current();
    openAnim = draw::approach(openAnim, isOpen ? 1.f : 0.f, 14.f * t.animSpeed);
    hubT = draw::approach(hubT, isOpen && current == Page::Hub ? 1.f : 0.f, 14.f * t.animSpeed);
    frameT = draw::approach(frameT, isOpen && current != Page::Hub ? 1.f : 0.f, 14.f * t.animSpeed);
    if (openAnim < 0.01f) return;

    Module* selectedBefore = selected;
    backdrop(std::clamp(openAnim, 0.f, 1.f));
    if (hubT > 0.01f) drawHub(hubT);
    if (frameT > 0.01f) drawFrame(frameT);
    theme::setFade(1.f);

    if (isOpen && ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !selectedBefore && !widgets::capturingKey()) {
        if (current == Page::Hub) setOpen(false);
        else go(Page::Hub);
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
