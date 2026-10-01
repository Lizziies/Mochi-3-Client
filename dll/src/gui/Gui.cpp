#include "Gui.hpp"
#include "I18n.hpp"
#include "HudEditor.hpp"
#include "Theme.hpp"
#include "Widgets.hpp"
#include "core/Build.hpp"
#include "core/Config.hpp"
#include "hook/Dx.hpp"
#include "modules/HudModule.hpp"
#include "modules/Manager.hpp"
#include "modules/client/ClickGui.hpp"
#include "render/Draw.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"
#include "server/Rules.hpp"
#include "sig/Sigs.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <map>
#include <random>
#include <vector>

namespace gui {

enum class Page { Modules, Themes, Profiles, Info };

static std::atomic<bool> isOpen{false};
static std::atomic<bool> hudEdit{false};
static std::atomic<bool> keyboardClaim{false};
static bool keyboardClaimNext = false;
static float openAnim = 0.f;
static Page page = Page::Modules;
static int category = -1;
static Module* selected = nullptr;
static Module* shown = nullptr;
static float panelAnim = 0.f;
static char search[64] = "";
static bool focusSearch = false;
static float gridT = 0.f;
static size_t gridKey = 0;

struct Particle {
    ImVec2 pos;
    ImVec2 vel;
    float life;
    float age = 0.f;
    float size;
};
static std::vector<Particle> particles;
static std::mt19937 rng{1337};

static float rnd(float a, float b) {
    return std::uniform_real_distribution<float>(a, b)(rng);
}

static void burst(ImVec2 at) {
    if (!theme::current().hearts) return;
    float s = ui::scale();
    for (int i = 0; i < 7; i++)
        particles.push_back({at, {rnd(-40, 40) * s, rnd(-120, -60) * s}, rnd(0.6f, 1.1f), 0.f, rnd(7, 12) * s});
}

static void drawParticles(ImDrawList* dl) {
    auto& t = theme::current();
    float dt = ui::dt();
    for (auto& p : particles) {
        p.age += dt;
        p.pos += p.vel * dt;
        p.vel.y += 60.f * ui::scale() * dt;
        float a = std::clamp(1.f - p.age / p.life, 0.f, 1.f);
        draw::heart(dl, p.pos, p.size * (0.6f + 0.4f * a), theme::col(theme::mix(t.accent, t.accent2, p.age / p.life), a));
    }
    std::erase_if(particles, [](const Particle& p) { return p.age >= p.life; });
}

void beginFrame() {
    keyboardClaim = keyboardClaimNext;
    keyboardClaimNext = false;
}

bool open() { return isOpen; }

void setOpen(bool on) {
    isOpen = on;
    if (on) {
        gridT = 0.f;
        hudEdit = false;
        focusSearch = true;
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
    setOpen(!isOpen);
}

void showModule(Module* m) {
    setOpen(true);
    page = Page::Modules;
    selected = m;
}

bool editingHud() { return hudEdit; }

void setEditingHud(bool on) {
    hudEdit = on;
    if (on) isOpen = false;
}

bool wantsInput() { return isOpen || hudEdit || keyboardClaim; }
bool wantsCursor() { return isOpen || hudEdit; }
bool capturesKeyboard() { return isOpen || widgets::capturingKey() || keyboardClaim || ImGui::GetIO().WantTextInput; }
void claimKeyboard() { keyboardClaimNext = true; }

static bool matches(const Module& m) {
    if (category >= 0 && (int)m.category() != category) return false;
    if (!search[0]) return true;
    auto lower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        return s;
    };
    std::string q = lower(search);
    if (lower(m.name()).find(q) != std::string::npos) return true;
    if (lower(m.description()).find(q) != std::string::npos) return true;
    for (auto& tag : m.tags())
        if (lower(tag).find(q) != std::string::npos) return true;
    return false;
}

static ImVec4 catColor(int cat) {
    static const unsigned pal[] = {0xFF8FBF, 0xB69CFF, 0xFF8A80, 0x8BE9B0, 0xFFD27E, 0x7EC8FF, 0xFFB38A, 0xFF7EB6};
    unsigned c = cat >= 0 && cat < 8 ? pal[cat] : 0xFF7EB6;
    return {((c >> 16) & 255) / 255.f, ((c >> 8) & 255) / 255.f, (c & 255) / 255.f, 1.f};
}

static void glyph(ImDrawList* dl, int kind, ImVec2 c, float r, ImU32 col) {
    float w = std::max(1.5f, r * 0.15f);
    switch (kind) {
    case -1:
        for (int i = 0; i < 4; i++) {
            float g = r * 0.38f;
            float x = c.x + ((i & 1) ? 1 : -1) * r * 0.52f, y = c.y + ((i & 2) ? 1 : -1) * r * 0.52f;
            dl->AddRectFilled({x - g, y - g}, {x + g, y + g}, col, g * 0.4f);
        }
        break;
    case 0:
        dl->AddRect({c.x - r, c.y - r * 0.72f}, {c.x + r, c.y + r * 0.72f}, col, r * 0.25f, 0, w);
        dl->AddLine({c.x - r * 0.55f, c.y - r * 0.15f}, {c.x + r * 0.15f, c.y - r * 0.15f}, col, w);
        dl->AddLine({c.x - r * 0.55f, c.y + r * 0.28f}, {c.x + r * 0.55f, c.y + r * 0.28f}, col, w);
        break;
    case 1:
        dl->AddEllipse(c, {r, r * 0.62f}, col, 0.f, 28, w);
        dl->AddCircleFilled(c, r * 0.3f, col, 16);
        break;
    case 2:
        dl->AddLine({c.x - r * 0.8f, c.y + r * 0.8f}, {c.x + r * 0.8f, c.y - r * 0.8f}, col, w * 1.3f);
        dl->AddLine({c.x + r * 0.8f, c.y + r * 0.8f}, {c.x - r * 0.8f, c.y - r * 0.8f}, col, w * 1.3f);
        dl->AddLine({c.x - r * 0.95f, c.y + r * 0.15f}, {c.x - r * 0.15f, c.y + r * 0.95f}, col, w);
        dl->AddLine({c.x + r * 0.95f, c.y + r * 0.15f}, {c.x + r * 0.15f, c.y + r * 0.95f}, col, w);
        break;
    case 3:
        for (int i = 0; i < 3; i++) {
            float y = c.y + (i - 1) * r * 0.62f;
            dl->AddLine({c.x - r, y}, {c.x + r, y}, col, w);
            dl->AddCircleFilled({c.x + (i == 1 ? 0.45f : -0.4f) * r, y}, r * 0.2f, col, 12);
        }
        break;
    case 4: {
        ImVec2 p[6] = {{c.x + r * 0.15f, c.y - r}, {c.x - r * 0.6f, c.y + r * 0.1f}, {c.x - r * 0.05f, c.y + r * 0.1f},
                       {c.x - r * 0.2f, c.y + r},  {c.x + r * 0.65f, c.y - r * 0.2f}, {c.x + r * 0.05f, c.y - r * 0.2f}};
        dl->AddConcavePolyFilled(p, 6, col);
        break;
    }
    case 5:
        for (int i = 0; i < 2; i++) {
            float y = c.y + (i ? 0.12f : -0.9f) * r;
            dl->AddRect({c.x - r, y}, {c.x + r, y + r * 0.78f}, col, r * 0.2f, 0, w);
            dl->AddCircleFilled({c.x - r * 0.55f, y + r * 0.39f}, r * 0.12f, col, 8);
        }
        break;
    case 6: draw::sparkle(dl, c, r, col); break;
    case 10:
        dl->AddCircle(c, r, col, 24, w);
        for (int i = 0; i < 3; i++) {
            float a = 3.6f + i * 1.15f;
            dl->AddCircleFilled({c.x + std::cos(a) * r * 0.5f, c.y + std::sin(a) * r * 0.5f}, r * 0.17f, col, 8);
        }
        break;
    case 11:
        dl->AddCircle({c.x, c.y - r * 0.38f}, r * 0.38f, col, 16, w);
        dl->PathArcTo({c.x, c.y + r * 0.95f}, r * 0.85f, 3.5f, 5.9f, 16);
        dl->PathStroke(col, 0, w);
        break;
    default:
        dl->AddCircle(c, r * 0.5f, col, 16, w);
        for (int i = 0; i < 8; i++) {
            float a = i * 0.7854f;
            dl->AddLine({c.x + std::cos(a) * r * 0.72f, c.y + std::sin(a) * r * 0.72f},
                        {c.x + std::cos(a) * r, c.y + std::sin(a) * r}, col, w * 1.4f);
        }
        break;
    }
}

static bool railItem(const char* label, int cat, bool active, int count, float width) {
    auto& t = theme::current();
    float s = ui::scale();
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 size{width, 38 * s};
    ImGui::PushID(label);
    bool clicked = ImGui::InvisibleButton("item", size);
    bool hovered = ImGui::IsItemHovered();
    float& a = *ImGui::GetStateStorage()->GetFloatRef(ImGui::GetItemID(), 0.f);
    ImGui::PopID();
    a = draw::approach(a, active || hovered ? 1.f : 0.f, 16.f * t.animSpeed);

    auto* dl = ImGui::GetWindowDrawList();
    if (!active && a > 0.01f) dl->AddRectFilled(p, p + size, theme::col(t.surfaceHover, a * 0.6f), t.rounding * 0.7f * s);
    float nudge = (active ? 0.f : a * 3.f) * s;
    ImVec4 gc = theme::mix(t.textDim, catColor(cat), active ? 1.f : a * 0.8f);
    glyph(dl, cat, {p.x + 24 * s + nudge, p.y + size.y * 0.5f}, 8 * s, theme::col(gc));
    const char* text = i18n::tr(label);
    ImVec2 ts = ImGui::CalcTextSize(text);
    dl->AddText({p.x + 46 * s + nudge, p.y + (size.y - ts.y) * 0.5f}, theme::col(theme::mix(t.textDim, t.text, active ? 1.f : a)), text);
    if (count > 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", count);
        ImVec2 cs = ImGui::CalcTextSize(buf);
        dl->AddText({p.x + size.x - cs.x - 12 * s, p.y + (size.y - cs.y) * 0.5f}, theme::col(t.textDim, 0.7f), buf);
    }
    return clicked;
}

static bool railIcon(const char* id, int kind, bool active, ImVec2 size, const char* tip) {
    auto& t = theme::current();
    float s = ui::scale();
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::PushID(id);
    bool clicked = ImGui::InvisibleButton("icon", size);
    bool hovered = ImGui::IsItemHovered();
    float& a = *ImGui::GetStateStorage()->GetFloatRef(ImGui::GetItemID(), 0.f);
    ImGui::PopID();
    a = draw::approach(a, active ? 1.f : (hovered ? 0.6f : 0.f), 16.f * t.animSpeed);

    auto* dl = ImGui::GetWindowDrawList();
    float r = t.rounding * 0.7f * s;
    if (active) dl->AddRectFilled(p, p + size, theme::col(t.accent, 0.22f), r);
    else if (a > 0.01f) dl->AddRectFilled(p, p + size, theme::col(t.surfaceHover, a), r);
    glyph(dl, kind, p + size * 0.5f, 9 * s, theme::col(theme::mix(t.textDim, active ? t.accent : t.text, std::max(a, active ? 1.f : 0.f))));
    if (hovered) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(i18n::tr(tip));
        ImGui::EndTooltip();
    }
    return clicked;
}

static void drawLogo(ImDrawList* dl, ImVec2 p) {
    auto& t = theme::current();
    float s = ui::scale();
    float bob = std::sin((float)ui::time() * 2.2f) * 1.5f * s;
    draw::glow(dl, p + ImVec2(4 * s, 4 * s + bob), p + ImVec2(28 * s, 28 * s + bob), 12 * s, theme::col(t.accent, 0.6f), 10 * s);
    draw::heart(dl, p + ImVec2(16 * s, 17 * s + bob), 30 * s, theme::col(t.accent));
    draw::heart(dl, p + ImVec2(12 * s, 13 * s + bob), 8 * s, theme::col(t.accent2, 0.9f));
    dl->AddText(fonts::bold(), 26 * s, p + ImVec2(38 * s, 1 * s), theme::col(t.text), build::name);
    if (t.sparkles) {
        float tw = 0.5f + 0.5f * std::sin((float)ui::time() * 3.1f);
        draw::sparkle(dl, p + ImVec2(34 * s, 0), 4 * s * tw + 1 * s, theme::col(t.accent2, tw));
    }
}

static void drawSidebar(float width) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 wp = ImGui::GetWindowPos();
    ImVec2 ws = ImGui::GetWindowSize();
    dl->AddRectFilled(wp, {wp.x + width, wp.y + ws.y}, theme::col(t.bg, 0.6f), t.rounding * s, ImDrawFlags_RoundCornersLeft);
    dl->AddLine({wp.x + width, wp.y + 16 * s}, {wp.x + width, wp.y + ws.y - 16 * s}, theme::col(t.surfaceHover, 0.6f), 1.f);

    ImGui::BeginChild("sidebar", {width, 0}, 0, ImGuiWindowFlags_NoBackground);
    ImGui::SetCursorPos({16 * s, 18 * s});
    drawLogo(ImGui::GetWindowDrawList(), ImGui::GetCursorScreenPos());
    ImGui::SetCursorPos({12 * s, 74 * s});
    ImGui::PushFont(fonts::bold(), 12.f * s);
    ImGui::TextColored(theme::mix(t.textDim, t.bg, 0.25f), "%s", i18n::tr("MODULES"));
    ImGui::PopFont();
    ImGui::SetCursorPosX(10 * s);

    struct Row {
        const char* label;
        int cat;
        int count;
    };
    static const Category order[] = {Category::Hud, Category::Visual, Category::Pvp, Category::Comfort,
                                     Category::Performance, Category::Server, Category::Fun};
    std::vector<Row> rows;
    int total = 0;
    for (auto c : order) {
        int n = (int)std::count_if(modules::all().begin(), modules::all().end(), [c](auto& m) { return m->category() == c; });
        if (!n) continue;
        total += n;
        rows.push_back({categoryName(c), (int)c, n});
    }
    rows.insert(rows.begin(), {"All modules", -1, total});

    float rowW = width - 20 * s;
    float step = 38 * s + ImGui::GetStyle().ItemSpacing.y;
    ImVec2 start = ImGui::GetCursorScreenPos();
    int active = 0;
    for (size_t i = 0; i < rows.size(); i++)
        if (rows[i].cat == category) active = (int)i;

    static float marker = -1.f;
    static float markerOn = 0.f;
    marker = marker < 0.f ? active * step : draw::approach(marker, active * step, 16.f * t.animSpeed);
    markerOn = draw::approach(markerOn, page == Page::Modules ? 1.f : 0.f, 14.f * t.animSpeed);
    if (markerOn > 0.01f) {
        ImVec2 a{start.x, start.y + marker}, b{start.x + rowW, start.y + marker + 38 * s};
        float r = t.rounding * 0.7f * s;
        if (t.gradient) draw::gradientRect(dl, a, b, theme::col(t.accent, 0.30f * markerOn), theme::col(t.accent2, 0.06f * markerOn), r);
        else dl->AddRectFilled(a, b, theme::col(t.accent, 0.2f * markerOn), r);
        dl->AddRectFilled({a.x, a.y + 10 * s}, {a.x + 3 * s, b.y - 10 * s}, theme::col(t.accent, markerOn), 2 * s);
    }
    for (size_t i = 0; i < rows.size(); i++) {
        if (railItem(rows[i].label, rows[i].cat, page == Page::Modules && (int)i == active, rows[i].count, rowW)) {
            page = Page::Modules;
            category = rows[i].cat;
            selected = nullptr;
        }
    }

    float footerY = ImGui::GetWindowHeight() - 58 * s;
    ImGui::SetCursorPos({10 * s, footerY});
    float gap = 6 * s;
    ImVec2 isz{(rowW - gap * 2) / 3.f, 40 * s};
    if (railIcon("themes", 10, page == Page::Themes, isz, "Appearance")) {
        page = Page::Themes;
        selected = nullptr;
    }
    ImGui::SameLine(0, gap);
    if (railIcon("profiles", 11, page == Page::Profiles, isz, "Profiles")) {
        page = Page::Profiles;
        selected = nullptr;
    }
    ImGui::SameLine(0, gap);
    if (railIcon("settings", 12, page == Page::Info, isz, "Settings")) {
        page = Page::Info;
        selected = nullptr;
    }
    ImGui::EndChild();
}

static void drawServerChip(ImVec2 rowMin, ImVec2 rowMax, float right) {
    auto& t = theme::current();
    float s = ui::scale();
    auto info = rules::status();
    std::string label = info.server.empty() ? std::string(i18n::tr("No server")) : info.server;
    if (info.blocked) label += "  ·  " + i18n::fmt("{} blocked", info.blocked);
    if (info.warned) label += "  ·  " + i18n::fmt("{} notices", info.warned);

    ImVec2 ts = ImGui::CalcTextSize(label.c_str());
    ImVec2 size{ts.x + 30 * s, 34 * s};
    ImVec2 p{right - size.x, rowMin.y + (rowMax.y - rowMin.y - size.y) * 0.5f};
    ImGui::SetCursorScreenPos(p);
    ImGui::InvisibleButton("server", size);
    auto* dl = ImGui::GetWindowDrawList();
    ImVec4 dot = info.server.empty() ? t.off : (info.blocked ? t.warn : t.ok);
    dl->AddRectFilled(p, p + size, theme::col(t.surface), size.y * 0.5f);
    dl->AddCircleFilled({p.x + 14 * s, p.y + size.y * 0.5f}, 4 * s, theme::col(dot));
    dl->AddText({p.x + 24 * s, p.y + (size.y - ts.y) * 0.5f}, theme::col(t.textDim), label.c_str());

    if (ImGui::IsItemHovered() && (!info.notes.empty() || !info.notice.empty())) {
        ImGui::BeginTooltip();
        if (!info.notice.empty()) ImGui::TextColored(t.warn, "%s", info.notice.c_str());
        for (auto& n : info.notes) ImGui::TextUnformatted(n.c_str());
        if (!info.rulesUrl.empty()) ImGui::TextColored(t.textDim, i18n::tr("Rules: %s"), info.rulesUrl.c_str());
        ImGui::EndTooltip();
    }
}

static void drawTopBar(float right) {
    auto& t = theme::current();
    float s = ui::scale();
    float h = 36 * s;
    ImGui::SetNextItemWidth(320 * s);
    if (focusSearch) {
        ImGui::SetKeyboardFocusHere();
        focusSearch = false;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 99.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {16 * s, (h - ImGui::GetFontSize()) * 0.5f});
    if (ImGui::InputTextWithHint("##search", i18n::tr("Search modules …"), search, sizeof(search)) && search[0]) {
        page = Page::Modules;
        selected = nullptr;
    }
    ImGui::PopStyleVar(2);
    ImVec2 rowMin = ImGui::GetItemRectMin(), rowMax = ImGui::GetItemRectMax();

    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 c0{right - 34 * s, rowMin.y + (rowMax.y - rowMin.y - 34 * s) * 0.5f};
    ImGui::SetCursorScreenPos(c0);
    bool closeClicked = ImGui::InvisibleButton("close", {34 * s, 34 * s});
    bool closeHover = ImGui::IsItemHovered();
    dl->AddRectFilled(c0, c0 + ImVec2(34 * s, 34 * s), theme::col(closeHover ? t.accent : t.surface, closeHover ? 0.9f : 1.f), 17 * s);
    ImVec2 cc = c0 + ImVec2(17 * s, 17 * s);
    ImU32 xc = theme::col(closeHover ? t.bg : t.textDim);
    dl->AddLine(cc + ImVec2(-5 * s, -5 * s), cc + ImVec2(5 * s, 5 * s), xc, 2.f * s);
    dl->AddLine(cc + ImVec2(5 * s, -5 * s), cc + ImVec2(-5 * s, 5 * s), xc, 2.f * s);
    if (closeClicked) setOpen(false);

    ImGui::SetCursorScreenPos({c0.x - 124 * s, c0.y});
    if (widgets::button("Edit HUD", {114 * s, 34 * s}, true)) setEditingHud(true);

    drawServerChip(rowMin, rowMax, c0.x - 136 * s);
    ImGui::SetCursorScreenPos({rowMin.x, rowMax.y + 14 * s});
}

static void smoothScroll() {
    ImGuiID id = ImGui::GetCurrentWindow()->ID;
    auto* store = ImGui::GetStateStorage();
    float& target = *store->GetFloatRef(id ^ 0x5CA1, 0.f);
    float& last = *store->GetFloatRef(id ^ 0x5CA2, 0.f);
    float cur = ImGui::GetScrollY();
    float maxY = ImGui::GetScrollMaxY();
    auto& io = ImGui::GetIO();

    if (std::fabs(cur - last) > 2.f) target = cur;
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && io.MouseWheel != 0.f)
        target -= io.MouseWheel * 84.f * ui::scale();
    target = std::clamp(target, 0.f, maxY);

    float next = draw::approach(cur, target, 13.f * theme::current().animSpeed);
    ImGui::SetScrollY(next);
    last = next;
}

static void drawCard(Module& m, ImVec2 size, bool isSelected) {
    auto& t = theme::current();
    float s = ui::scale();
    ImGui::PushID(m.name().c_str());
    ImVec2 p = ImGui::GetCursorScreenPos();

    bool locked = !m.available() || m.rule() == RuleLevel::Block;
    bool clicked = ImGui::InvisibleButton("card", size);
    bool hovered = ImGui::IsItemHovered();
    m.hover = draw::approach(m.hover, hovered ? 1.f : 0.f, 14.f * t.animSpeed);
    m.anim = draw::approach(m.anim, m.enabled() ? 1.f : 0.f, 12.f * t.animSpeed);

    auto* dl = ImGui::GetWindowDrawList();
    float r = t.rounding * 0.8f * s;
    float lift = m.hover * 2 * s;
    ImVec2 min = p - ImVec2(0, lift), max = p + size - ImVec2(0, lift);
    float dim = locked ? 0.5f : 1.f;

    if (m.anim > 0.01f) draw::glow(dl, min, max, r, theme::col(t.accent, 0.4f * m.anim), 7 * s);
    ImVec4 base = theme::mix(t.surface, t.surfaceHover, m.hover);
    if (t.gradient && m.anim > 0.01f)
        draw::gradientRect(dl, min, max, theme::col(theme::mix(base, t.accent, 0.16f * m.anim)), theme::col(base), r);
    else
        dl->AddRectFilled(min, max, theme::col(base, locked ? 0.6f : 1.f), r);
    if (isSelected) dl->AddRect(min, max, theme::col(t.accent), r, 0, 2.f * s);
    else if (m.anim > 0.01f) dl->AddRect(min, max, theme::col(t.accent, 0.5f * m.anim), r, 0, 1.2f * s);

    float pad = 14 * s;
    float badge = 46 * s;
    ImVec2 bmin = min + ImVec2(pad, (size.y - badge) * 0.5f);
    ImVec4 cc = catColor((int)m.category());
    draw::gradientRect(dl, bmin, bmin + ImVec2(badge, badge), theme::col(cc, 0.95f * dim), theme::col(theme::mix(cc, t.accent2, 0.5f), 0.95f * dim), 12 * s);
    glyph(dl, (int)m.category(), bmin + ImVec2(badge, badge) * 0.5f, 10 * s, theme::col(t.bg, 0.9f));

    float tx = min.x + pad + badge + 14 * s;
    float tw = max.x - tx - pad - 46 * s;
    ImVec4 title = locked ? t.textDim : t.text;
    dl->AddText(fonts::bold(), 17 * s, {tx, min.y + pad - 1 * s}, theme::col(title), i18n::tr(m.name().c_str()));

    std::string desc = i18n::tr(m.description().c_str());
    if (!m.available()) desc = i18n::tr("Not available on this Minecraft version yet.");
    else if (m.rule() == RuleLevel::Block) desc = m.ruleNote();
    dl->PushClipRect({tx, min.y + pad + 22 * s}, {tx + tw, max.y - 8 * s}, true);
    dl->AddText(fonts::regular(), 13.5f * s, {tx, min.y + pad + 23 * s}, theme::col(t.textDim), desc.c_str(), nullptr, tw);
    dl->PopClipRect();

    ImVec2 tp = {max.x - pad - 40 * s, min.y + pad - 1 * s};
    if (m.rule() == RuleLevel::Warn || m.risky()) {
        ImVec2 c = {tp.x - 14 * s, tp.y + 11 * s};
        dl->AddTriangleFilled({c.x, c.y - 7 * s}, {c.x - 7 * s, c.y + 6 * s}, {c.x + 7 * s, c.y + 6 * s}, theme::col(t.warn));
        dl->AddText(fonts::bold(), 11 * s, {c.x - 1.5f * s, c.y - 4 * s}, theme::col(t.bg), "!");
    }
    ImGui::SetCursorScreenPos(tp);
    bool on = m.userEnabled() && !locked;
    if (locked) {
        ImVec2 lp = tp + ImVec2(20 * s, 11 * s);
        dl->AddRectFilled(lp + ImVec2(-6 * s, -1 * s), lp + ImVec2(6 * s, 8 * s), theme::col(t.off), 2 * s);
        dl->AddCircle(lp + ImVec2(0, -2 * s), 4 * s, theme::col(t.off), 12, 2 * s);
    } else if (!m.alwaysOn() && widgets::toggle("t", on)) {
        m.setEnabled(on);
        if (m.enabled()) burst(tp + ImVec2(20 * s, 0));
    }

    int key = m.keybind().i;
    if (key && !locked) {
        std::string kn = widgets::keyName(key);
        ImVec2 ks = ImGui::CalcTextSize(kn.c_str());
        ImVec2 kp{max.x - pad - ks.x - 14 * s, max.y - pad - ks.y - 6 * s};
        dl->AddRectFilled(kp, kp + ks + ImVec2(14 * s, 6 * s), theme::col(t.bg, 0.5f), 8 * s);
        dl->AddText(kp + ImVec2(7 * s, 3 * s), theme::col(t.textDim), kn.c_str());
    }

    if (hovered && (m.rule() == RuleLevel::Warn || m.risky()) && !m.ruleNote().empty()) {
        ImGui::BeginTooltip();
        ImGui::TextColored(t.warn, "%s", m.ruleNote().c_str());
        ImGui::EndTooltip();
    }

    if (clicked && !locked) selected = isSelected ? nullptr : &m;
    ImGui::SetCursorScreenPos(p + ImVec2(0, size.y));
    ImGui::PopID();
}

static void drawGrid(float width) {
    float s = ui::scale();
    auto& t = theme::current();
    ImGui::BeginChild("grid", {width, 0}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);

    size_t key = std::hash<std::string>{}(search) * 31 + size_t(category + 2);
    if (key != gridKey) {
        gridKey = key;
        gridT = 0.f;
        ImGui::SetScrollY(0.f);
    }
    gridT += ui::dt();
    smoothScroll();

    float avail = ImGui::GetContentRegionAvail().x;
    float gap = 12 * s;
    int cols = std::max(1, int((avail + gap) / (290 * s + gap)));
    float w = (avail - gap * (cols - 1)) / cols;
    ImVec2 size{w, 88 * s};

    std::vector<Module*> list;
    for (auto& m : modules::all())
        if (matches(*m) && m->category() != Category::Client) list.push_back(m.get());

    std::map<std::string, int> subRank;
    for (auto* m : list) subRank.emplace(std::to_string((int)m->category()) + "|" + m->sub(), (int)subRank.size());
    std::stable_sort(list.begin(), list.end(), [&](Module* a, Module* b) {
        if (a->category() != b->category()) return (int)a->category() < (int)b->category();
        return subRank[std::to_string((int)a->category()) + "|" + a->sub()] < subRank[std::to_string((int)b->category()) + "|" + b->sub()];
    });

    if (list.empty()) widgets::hint("Nothing found.");

    float menuFade = theme::fade();
    ImVec2 origin = ImGui::GetCursorScreenPos();
    float y = 2 * s;
    int col = 0;
    std::string lastHeader = "\x01";
    for (size_t i = 0; i < list.size(); i++) {
        Module* m = list[i];
        std::string header = category < 0 ? std::string(categoryName(m->category())) : std::string();
        if (!m->sub().empty()) header += (header.empty() ? "" : "  ·  ") + std::string(i18n::tr(m->sub().c_str()));
        if (header != lastHeader) {
            if (col != 0) {
                y += size.y + gap;
                col = 0;
            }
            if (!header.empty()) {
                if (i) y += 6 * s;
                ImGui::GetWindowDrawList()->AddText(fonts::bold(), 13.f * s, origin + ImVec2(2 * s, y), theme::col(theme::mix(t.textDim, catColor((int)m->category()), 0.6f)), header.c_str());
                y += 24 * s;
            }
            lastHeader = header;
        }
        float e = draw::easeOutCubic(std::clamp((gridT - float(i) * 0.012f) / 0.3f, 0.f, 1.f));
        ImGui::SetCursorScreenPos(origin + ImVec2(col * (w + gap), y + (1.f - e) * 14 * s));
        theme::setFade(menuFade * e);
        drawCard(*m, size, selected == m);
        if (++col >= cols) {
            col = 0;
            y += size.y + gap;
        }
    }
    theme::setFade(menuFade);
    if (col != 0) y += size.y + gap;
    ImGui::SetCursorScreenPos(origin + ImVec2(0, y));
    ImGui::Dummy({0, 8 * s});
    ImGui::EndChild();
}

static void drawSettingsPanel(ImVec2 origin, ImVec2 size) {
    auto& t = theme::current();
    float s = ui::scale();
    if (!shown) return;
    Module& m = *shown;

    float e = draw::easeOutCubic(panelAnim);
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
        ImVec4 cc = catColor((int)m.category());
        draw::gradientRect(dl, hp, hp + ImVec2(badge, badge), theme::col(cc), theme::col(theme::mix(cc, t.accent2, 0.5f)), 12 * s);
        glyph(dl, (int)m.category(), hp + ImVec2(badge, badge) * 0.5f, 10 * s, theme::col(t.bg, 0.9f));
        dl->AddText(fonts::bold(), 22 * s, hp + ImVec2(badge + 14 * s, 1 * s), theme::col(t.text), i18n::tr(m.name().c_str()));
        std::string cat = categoryName(m.category());
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
        if (closeClicked) selected = nullptr;
        ImGui::SetCursorScreenPos(hp + ImVec2(0, badge + 12 * s));
    }
    widgets::hint(m.description().c_str());
    if (!m.ruleNote().empty()) ImGui::TextColored(t.warn, "%s", m.ruleNote().c_str());
    ImGui::Dummy({0, 4 * s});
    ImGui::Separator();

    if (!m.alwaysOn()) {
        bool on = m.userEnabled();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(i18n::tr("Active"));
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - 40 * s);
        if (widgets::toggle("enabled", on)) m.setEnabled(on);
    }

    auto* hud = dynamic_cast<HudModule*>(&m);
    auto isStyle = [hud](const Setting& st) {
        static const char* ids[] = {"bg", "bgColor", "textColor", "accent", "rounding", "padding", "shadow", "scale"};
        if (!hud) return false;
        for (auto id : ids)
            if (st.id == id) return true;
        return false;
    };

    for (auto& set : m.settings()) {
        if (set.type == SettingType::Key && m.alwaysOn() && set.id != "key") continue;
        if (isStyle(set)) continue;
        widgets::setting(set);
    }
    m.drawSettings();

    if (hud) {
        static bool styleOpen = false;
        static float styleAnim = 0.f;
        ImGui::Dummy({0, 6 * s});
        if (widgets::button(styleOpen ? "Hide style options" : "Style options")) styleOpen = !styleOpen;
        styleAnim = draw::approach(styleAnim, styleOpen ? 1.f : 0.f, 14.f * t.animSpeed);
        if (styleAnim > 0.01f) {
            float outer = theme::fade();
            theme::setFade(outer * styleAnim);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * styleAnim);
            for (auto& set : m.settings())
                if (isStyle(set)) widgets::setting(set);
            ImGui::PopStyleVar();
            theme::setFade(outer);
        }
        ImGui::Dummy({0, 6 * s});
        if (widgets::button("Change position in the HUD editor")) setEditingHud(true);
        ImGui::SameLine();
        if (widgets::button("Reset")) {
            hud->setPosition({20 * s, 120 * s});
            hud->setScale(1.f);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(2);
    if (selected && ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !widgets::capturingKey()) selected = nullptr;
}

static void colorRow(const char* label, ImVec4& c) {
    float col[4] = {c.x, c.y, c.z, c.w};
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(i18n::tr(label));
    ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.5f);
    if (ImGui::ColorEdit4((std::string("##") + label).c_str(), col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar)) {
        c = {col[0], col[1], col[2], col[3]};
        theme::applyStyle();
        config::markDirty();
    }
}

static void drawThemes() {
    auto& t = theme::current();
    float s = ui::scale();
    ImGui::BeginChild("themes", {std::min(ImGui::GetContentRegionAvail().x, 780 * ui::scale()), 0}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
    smoothScroll();
    widgets::sectionTitle("Presets");

    float avail = ImGui::GetContentRegionAvail().x;
    float gap = 10 * s;
    int cols = std::max(1, int((avail + gap) / (160 * s + gap)));
    float w = (avail - gap * (cols - 1)) / cols;
    ImVec2 origin = ImGui::GetCursorScreenPos();
    auto& presets = theme::presets();
    for (size_t i = 0; i < presets.size(); i++) {
        auto& p = presets[i];
        ImVec2 pos = origin + ImVec2((i % cols) * (w + gap), (i / cols) * (74 * s + gap));
        ImGui::SetCursorScreenPos(pos);
        ImGui::PushID((int)i);
        bool clicked = ImGui::InvisibleButton("preset", {w, 74 * s});
        bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();
        auto* dl = ImGui::GetWindowDrawList();
        float r = t.rounding * 0.7f * s;
        dl->AddRectFilled(pos, pos + ImVec2(w, 74 * s), theme::col(p.bg), r);
        if (t.name == p.name) dl->AddRect(pos, pos + ImVec2(w, 74 * s), theme::col(t.accent), r, 0, 2 * s);
        else if (hovered) dl->AddRect(pos, pos + ImVec2(w, 74 * s), theme::col(p.accent, 0.6f), r, 0, 1.5f * s);
        draw::gradientRect(dl, pos + ImVec2(12 * s, 14 * s), pos + ImVec2(w - 12 * s, 30 * s), theme::col(p.accent),
                           theme::col(p.accent2), 8 * s);
        draw::heart(dl, pos + ImVec2(20 * s, 52 * s), 16 * s, theme::col(p.accent));
        dl->AddText(fonts::bold(), 16 * s, pos + ImVec2(34 * s, 43 * s), theme::col(p.text), p.name.c_str());
        if (clicked) theme::use(p);
    }
    int rows = int((presets.size() + cols - 1) / cols);
    ImGui::SetCursorScreenPos(origin + ImVec2(0, rows * (74 * s + gap)));

    widgets::sectionTitle("Custom theme");
    colorRow("Background", t.bg);
    colorRow("Surfaces", t.surface);
    colorRow("Surfaces (hover)", t.surfaceHover);
    colorRow("Accent", t.accent);
    colorRow("Accent 2", t.accent2);
    colorRow("Text", t.text);
    colorRow("Dimmed text", t.textDim);
    colorRow("Success", t.ok);
    colorRow("Warning", t.warn);
    colorRow("Off", t.off);

    auto sliderRow = [&](const char* label, float& v, float a, float b, const char* fmt) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(i18n::tr(label));
        ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.5f);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::SliderFloat((std::string("##") + label).c_str(), &v, a, b, fmt)) {
            theme::applyStyle();
            config::markDirty();
        }
    };
    sliderRow("Corner radius", t.rounding, 0.f, 24.f, "%.0f");
    sliderRow("Opacity", t.opacity, 0.5f, 1.f, "%.2f");
    sliderRow("Animation speed", t.animSpeed, 0.25f, 3.f, "%.2fx");

    auto toggleRow = [&](const char* label, bool& v) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(i18n::tr(label));
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - 40 * s);
        widgets::toggle(label, v);
    };
    toggleRow("Gradients", t.gradient);
    toggleRow("Sparkles", t.sparkles);
    toggleRow("Little hearts", t.hearts);

    ImGui::Dummy({0, 6 * s});
    if (widgets::button("Copy theme as code", {0, 0}, true)) ImGui::SetClipboardText(theme::exportCode().c_str());
    ImGui::SameLine();
    if (widgets::button("Load code from clipboard")) {
        const char* clip = ImGui::GetClipboardText();
        theme::importCode(clip ? clip : "");
    }
    ImGui::EndChild();
}

static void drawProfiles() {
    float s = ui::scale();
    auto& t = theme::current();
    ImGui::BeginChild("profiles", {std::min(ImGui::GetContentRegionAvail().x, 780 * ui::scale()), 0}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
    smoothScroll();
    widgets::sectionTitle("Profile");
    widgets::hint("Each profile stores modules, settings, HUD positions and theme.");
    ImGui::Dummy({0, 4 * s});

    for (auto& name : config::profiles()) {
        ImGui::PushID(name.c_str());
        bool active = name == config::profile();
        ImGui::AlignTextToFramePadding();
        if (active) ImGui::TextColored(t.accent, i18n::tr("%s  (active)"), name.c_str());
        else ImGui::TextUnformatted(name.c_str());
        ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.6f);
        if (!active) {
            if (widgets::button("Load")) config::switchProfile(name);
            ImGui::SameLine();
            if (widgets::button("Delete")) config::deleteProfile(name);
        }
        ImGui::PopID();
    }

    static char newName[32] = "";
    ImGui::Dummy({0, 8 * s});
    ImGui::SetNextItemWidth(220 * s);
    ImGui::InputTextWithHint("##new", i18n::tr("New profile …"), newName, sizeof(newName));
    ImGui::SameLine();
    if (widgets::button("Create", {0, 0}, true) && newName[0]) {
        config::switchProfile(newName);
        newName[0] = 0;
    }
    ImGui::EndChild();
}

static void drawInfo() {
    auto& t = theme::current();
    ImGui::BeginChild("info", {std::min(ImGui::GetContentRegionAvail().x, 780 * ui::scale()), 0}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
    smoothScroll();
    widgets::sectionTitle("Mochi");
    ImGui::Text(i18n::tr("Version %s"), build::version);
    ImGui::Text(i18n::tr("Renderer: %s"), dx::api() == dx::Api::Dx12 ? "DirectX 12" : dx::api() == dx::Api::Dx11 ? "DirectX 11" : "–");

    widgets::sectionTitle("Minecraft");
    auto st = sigs::stats();
    ImGui::Text(i18n::tr("Version: %s"), i18n::tr(st.gameVersion.c_str()));
    ImGui::Text(i18n::tr("Signatures: %d of %d found"), st.found, st.total);
    ImGui::TextColored(t.textDim, i18n::tr("Source: %s"), i18n::tr(st.source.c_str()));
    int unavailable = 0;
    for (auto& m : modules::all())
        if (!m->available()) unavailable++;
    if (unavailable) ImGui::TextColored(t.warn, i18n::tr("%d modules are waiting for signatures for this version."), unavailable);

    widgets::sectionTitle("Server");
    auto rs = rules::status();
    ImGui::Text(i18n::tr("Connected: %s"), rs.server.empty() ? "–" : rs.server.c_str());
    if (!rs.host.empty()) ImGui::TextColored(t.textDim, "%s", rs.host.c_str());
    ImGui::TextColored(t.textDim, i18n::tr("Rules: %s"), i18n::tr(rules::source().c_str()));

    widgets::sectionTitle("Language");
    widgets::hint("Auto follows your Windows language.");
    const char* names[] = {"Auto", "English", "Deutsch"};
    for (int i = 0; i < 3; i++) {
        if (i) ImGui::SameLine();
        if (widgets::button(names[i], {0, 0}, int(i18n::chosen()) == i)) i18n::choose(i18n::Lang(i));
    }

    widgets::sectionTitle("Keys");
    if (auto* menu = modules::get<ClickGui>()) widgets::setting(menu->keybind());
    widgets::hint("Ctrl+L: unload client · F1: hide HUD · ESC: back · right-click a key: clear");
    ImGui::EndChild();
}

static void drawMenu() {
    auto& t = theme::current();
    float s = ui::scale();
    auto ds = ImGui::GetIO().DisplaySize;

    openAnim = draw::approach(openAnim, isOpen ? 1.f : 0.f, 14.f * t.animSpeed);
    if (openAnim < 0.01f) return;

    float menuFade = std::clamp(openAnim, 0.f, 1.f);
    auto* bg = ImGui::GetBackgroundDrawList();
    bg->AddRectFilledMultiColor({0, 0}, ds, IM_COL32(8, 3, 12, int(150 * menuFade)), IM_COL32(8, 3, 12, int(150 * menuFade)),
                                IM_COL32(28, 8, 36, int(215 * menuFade)), IM_COL32(28, 8, 36, int(215 * menuFade)));
    theme::setFade(menuFade);

    static Page lastPage = Page::Modules;
    static float pageT = 1.f;
    if (page != lastPage) {
        lastPage = page;
        pageT = 0.f;
        gridT = 0.f;
    }
    pageT = std::min(1.f, pageT + ui::dt() / 0.28f);

    ImVec2 size{std::min(1560 * s, ds.x * 0.86f), std::min(900 * s, ds.y * 0.84f)};
    float e = draw::easeOutBack(openAnim);
    ImVec2 pos = (ds - size) * 0.5f + ImVec2(0, (1.f - e) * 24 * s);

    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, menuFade);
    ImGui::Begin("##mochi", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);

    auto* dl = ImGui::GetWindowDrawList();
    draw::glow(dl, pos, pos + size, t.rounding * s, theme::col(t.accent, 0.22f), 22 * s);
    dl->AddRect(pos, pos + size, theme::col(t.accent, 0.25f), t.rounding * s, 0, 1.2f);

    float rail = 224 * s;
    drawSidebar(rail);
    ImGui::SameLine(rail + 24 * s);

    float contentW = size.x - rail - 44 * s;
    float paneW = std::min(430 * s, contentW * 0.42f);
    if (selected && page == Page::Modules) shown = selected;
    panelAnim = draw::approach(panelAnim, selected && page == Page::Modules ? 1.f : 0.f, 14.f * t.animSpeed);
    float gridW = contentW - (paneW + 16 * s) * draw::easeOutCubic(panelAnim);

    ImGui::BeginGroup();
    float pageE = draw::easeOutCubic(pageT);
    theme::setFade(menuFade * (0.25f + 0.75f * pageE));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, menuFade * (0.25f + 0.75f * pageE));
    drawTopBar(pos.x + size.x - 22 * s);
    switch (page) {
    case Page::Modules: drawGrid(gridW); break;
    case Page::Themes: drawThemes(); break;
    case Page::Profiles: drawProfiles(); break;
    case Page::Info: drawInfo(); break;
    }
    ImGui::PopStyleVar();
    theme::setFade(menuFade);
    ImGui::EndGroup();

    drawParticles(ImGui::GetForegroundDrawList());
    ImGui::End();

    if (panelAnim > 0.01f && shown) {
        float top = 72 * s;
        drawSettingsPanel({pos.x + size.x - paneW - 20 * s, pos.y + top}, {paneW, size.y - top - 20 * s});
    } else {
        shown = nullptr;
    }
    ImGui::PopStyleVar();
    theme::setFade(1.f);

    if (isOpen && ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !selected && !widgets::capturingKey()) setOpen(false);
}

void draw() {
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
