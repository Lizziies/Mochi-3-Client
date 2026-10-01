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

static bool sidebarItem(const char* label, bool active, float width) {
    auto& t = theme::current();
    float s = ui::scale();
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 size{width, 36 * s};
    bool clicked = ImGui::InvisibleButton(label, size);
    bool hovered = ImGui::IsItemHovered();

    float& a = *ImGui::GetStateStorage()->GetFloatRef(ImGui::GetItemID(), 0.f);
    a = draw::approach(a, active ? 1.f : (hovered ? 0.35f : 0.f), 16.f * t.animSpeed);

    auto* dl = ImGui::GetWindowDrawList();
    float r = t.rounding * 0.7f * s;
    if (a > 0.01f) {
        if (active && t.gradient)
            draw::gradientRect(dl, p, p + size, theme::col(t.accent, 0.28f * a), theme::col(t.accent2, 0.08f * a), r);
        else
            dl->AddRectFilled(p, p + size, theme::col(t.surfaceHover, a), r);
    }
    if (active) dl->AddRectFilled({p.x, p.y + 9 * s}, {p.x + 3 * s, p.y + size.y - 9 * s}, theme::col(t.accent), 2 * s);
    const char* text = i18n::tr(label);
    ImVec2 ts = ImGui::CalcTextSize(text);
    dl->AddText({p.x + 16 * s, p.y + (size.y - ts.y) * 0.5f}, theme::col(active ? t.text : t.textDim), text);
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
    dl->AddRectFilled(wp, {wp.x + width + 16 * s, wp.y + ws.y}, theme::col(t.surface, 0.55f), t.rounding * s,
                      ImDrawFlags_RoundCornersLeft);

    ImGui::BeginChild("sidebar", {width, 0}, 0, ImGuiWindowFlags_NoBackground);
    drawLogo(ImGui::GetWindowDrawList(), ImGui::GetCursorScreenPos());
    ImGui::Dummy({0, 44 * s});

    if (sidebarItem("All modules", page == Page::Modules && category < 0, width)) {
        page = Page::Modules;
        category = -1;
        selected = nullptr;
    }
    static const Category cats[] = {Category::Hud, Category::Visual, Category::Pvp, Category::Comfort,
                                    Category::Performance, Category::Server, Category::Fun};
    for (auto c : cats) {
        bool any = std::any_of(modules::all().begin(), modules::all().end(),
                               [c](auto& m) { return m->category() == c; });
        if (!any) continue;
        if (sidebarItem(categoryName(c), page == Page::Modules && category == (int)c, width)) {
            page = Page::Modules;
            category = (int)c;
            selected = nullptr;
        }
    }

    ImGui::Dummy({0, 10 * s});
    if (sidebarItem("Themes", page == Page::Themes, width)) {
        page = Page::Themes;
        selected = nullptr;
    }
    if (sidebarItem("Profile", page == Page::Profiles, width)) {
        page = Page::Profiles;
        selected = nullptr;
    }
    if (sidebarItem("Info", page == Page::Info, width)) {
        page = Page::Info;
        selected = nullptr;
    }

    float bottom = ImGui::GetWindowHeight() - 44 * s;
    if (ImGui::GetCursorPosY() < bottom) ImGui::SetCursorPosY(bottom);
    if (widgets::button("Edit HUD", {width, 38 * s}, true)) setEditingHud(true);
    ImGui::EndChild();
}

static void drawServerChip(ImVec2 rowMin, ImVec2 rowMax) {
    auto& t = theme::current();
    float s = ui::scale();
    auto info = rules::status();
    std::string label = info.server.empty() ? std::string(i18n::tr("No server")) : info.server;
    if (info.blocked) label += "  ·  " + i18n::fmt("{} blocked", info.blocked);
    if (info.warned) label += "  ·  " + i18n::fmt("{} notices", info.warned);

    ImVec2 ts = ImGui::CalcTextSize(label.c_str());
    ImVec2 size{ts.x + 30 * s, ImGui::GetFrameHeight()};
    float right = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x;
    ImVec2 p{right - size.x, rowMin.y + (rowMax.y - rowMin.y - size.y) * 0.5f};
    ImGui::SetCursorScreenPos(p);
    ImGui::InvisibleButton("server", size);
    auto* dl = ImGui::GetWindowDrawList();
    ImVec4 dot = info.server.empty() ? t.off : (info.blocked ? t.warn : t.ok);
    dl->AddRectFilled(p, p + size, theme::col(t.surface), size.y * 0.5f);
    dl->AddCircleFilled({p.x + 13 * s, p.y + size.y * 0.5f}, 4 * s, theme::col(dot));
    dl->AddText({p.x + 22 * s, p.y + (size.y - ts.y) * 0.5f}, theme::col(t.textDim), label.c_str());

    if (ImGui::IsItemHovered() && (!info.notes.empty() || !info.notice.empty())) {
        ImGui::BeginTooltip();
        if (!info.notice.empty()) ImGui::TextColored(t.warn, "%s", info.notice.c_str());
        for (auto& n : info.notes) ImGui::TextUnformatted(n.c_str());
        if (!info.rulesUrl.empty()) ImGui::TextColored(t.textDim, i18n::tr("Rules: %s"), info.rulesUrl.c_str());
        ImGui::EndTooltip();
    }
}

static void drawTopBar() {
    float s = ui::scale();
    ImGui::SetNextItemWidth(300 * s);
    if (focusSearch) {
        ImGui::SetKeyboardFocusHere();
        focusSearch = false;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 99.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {14 * s, 8 * s});
    if (ImGui::InputTextWithHint("##search", i18n::tr("Search modules …"), search, sizeof(search)) && search[0]) {
        page = Page::Modules;
        selected = nullptr;
    }
    ImGui::PopStyleVar(2);
    ImVec2 rowMin = ImGui::GetItemRectMin(), rowMax = ImGui::GetItemRectMax();
    drawServerChip(rowMin, rowMax);
    ImGui::SetCursorScreenPos({rowMin.x, rowMax.y + 12 * s});
}

static void drawCard(Module& m, ImVec2 size) {
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

    if (m.anim > 0.01f) draw::glow(dl, min, max, r, theme::col(t.accent, 0.5f * m.anim), 7 * s);
    dl->AddRectFilled(min, max, theme::col(theme::mix(t.surface, t.surfaceHover, m.hover), locked ? 0.55f : 1.f), r);
    if (m.anim > 0.01f) dl->AddRect(min, max, theme::col(t.accent, 0.7f * m.anim), r, 0, 1.5f * s);

    float pad = 14 * s;
    ImVec4 title = locked ? t.textDim : t.text;
    dl->AddText(fonts::bold(), 18 * s, min + ImVec2(pad, pad - 2 * s), theme::col(title), i18n::tr(m.name().c_str()));

    std::string desc = i18n::tr(m.description().c_str());
    if (!m.available()) desc = i18n::tr("Not available on this Minecraft version yet.");
    else if (m.rule() == RuleLevel::Block) desc = m.ruleNote();
    dl->AddText(fonts::regular(), 14.5f * s, min + ImVec2(pad, pad + 22 * s), theme::col(t.textDim), desc.c_str(),
                nullptr, size.x - pad * 2 - 10 * s);

    if (m.rule() == RuleLevel::Warn || m.risky()) {
        ImVec2 c = {max.x - pad - 50 * s, min.y + pad + 9 * s};
        dl->AddTriangleFilled({c.x, c.y - 7 * s}, {c.x - 7 * s, c.y + 6 * s}, {c.x + 7 * s, c.y + 6 * s}, theme::col(t.warn));
        dl->AddText(fonts::bold(), 11 * s, {c.x - 1.5f * s, c.y - 4 * s}, theme::col(t.bg), "!");
    }

    ImGui::SetCursorScreenPos({max.x - pad - 40 * s, min.y + pad - 1 * s});
    bool on = m.userEnabled() && !locked;
    if (locked) {
        ImVec2 lp = ImGui::GetCursorScreenPos() + ImVec2(20 * s, 10 * s);
        dl->AddRectFilled(lp + ImVec2(-6 * s, -1 * s), lp + ImVec2(6 * s, 8 * s), theme::col(t.off), 2 * s);
        dl->AddCircle(lp + ImVec2(0, -2 * s), 4 * s, theme::col(t.off), 12, 2 * s);
    } else if (!m.alwaysOn() && widgets::toggle("t", on)) {
        m.setEnabled(on);
        if (m.enabled()) burst(ImGui::GetCursorScreenPos() + ImVec2(-20 * s, 0));
    }

    if (hovered && (m.rule() == RuleLevel::Warn || m.risky()) && !m.ruleNote().empty()) {
        ImGui::BeginTooltip();
        ImGui::TextColored(t.warn, "%s", m.ruleNote().c_str());
        ImGui::EndTooltip();
    }

    if (clicked && !locked) selected = &m;
    ImGui::SetCursorScreenPos(p + ImVec2(0, size.y));
    ImGui::PopID();
}

static void drawGrid() {
    float s = ui::scale();
    ImGui::BeginChild("grid", {0, 0}, 0, ImGuiWindowFlags_NoBackground);
    float avail = ImGui::GetContentRegionAvail().x;
    float gap = 12 * s;
    int cols = std::max(1, int((avail + gap) / (250 * s + gap)));
    float w = (avail - gap * (cols - 1)) / cols;
    ImVec2 size{w, 96 * s};

    std::vector<Module*> shown;
    for (auto& m : modules::all())
        if (matches(*m) && m->category() != Category::Client) shown.push_back(m.get());

    if (shown.empty()) widgets::hint("Nothing found.");

    ImVec2 origin = ImGui::GetCursorScreenPos();
    for (size_t i = 0; i < shown.size(); i++) {
        int col = int(i % cols), row = int(i / cols);
        ImGui::SetCursorScreenPos(origin + ImVec2(col * (w + gap), row * (size.y + gap) + 3 * s));
        drawCard(*shown[i], size);
    }
    ImGui::Dummy({0, gap});
    ImGui::EndChild();
}

static void drawSettingsPanel(ImVec2 origin, ImVec2 size) {
    auto& t = theme::current();
    float s = ui::scale();
    if (!shown) return;
    Module& m = *shown;

    float off = (1.f - draw::easeOutCubic(panelAnim)) * size.x;
    ImGui::SetNextWindowPos({origin.x + off, origin.y});
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme::col(t.surface, 0.99f));
    ImGui::PushStyleColor(ImGuiCol_Border, theme::col(t.accent, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, t.rounding * s);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {18 * s, 14 * s});
    ImGui::Begin("##mochi_panel", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing);

    if (widgets::button("‹ Back")) selected = nullptr;
    ImGui::SameLine();
    ImGui::PushFont(fonts::bold(), 22.f);
    ImGui::TextUnformatted(i18n::tr(m.name().c_str()));
    ImGui::PopFont();
    widgets::hint(m.description().c_str());
    if (!m.ruleNote().empty()) ImGui::TextColored(t.warn, "%s", m.ruleNote().c_str());
    ImGui::Separator();

    if (!m.alwaysOn()) {
        bool on = m.userEnabled();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(i18n::tr("Active"));
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - 40 * s);
        if (widgets::toggle("enabled", on)) m.setEnabled(on);
    }

    for (auto& set : m.settings()) {
        if (set.type == SettingType::Key && m.alwaysOn() && set.id != "key") continue;
        widgets::setting(set);
    }
    m.drawSettings();

    if (auto* hud = dynamic_cast<HudModule*>(&m)) {
        ImGui::Dummy({0, 6 * s});
        if (widgets::button("Change position in the HUD editor")) setEditingHud(true);
        ImGui::SameLine();
        if (widgets::button("Reset")) {
            hud->setPosition({20 * s, 120 * s});
            hud->setScale(1.f);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(3);
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
    ImGui::BeginChild("themes", {0, 0}, 0, ImGuiWindowFlags_NoBackground);
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
    ImGui::BeginChild("profiles", {0, 0}, 0, ImGuiWindowFlags_NoBackground);
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
    ImGui::BeginChild("info", {0, 0}, 0, ImGuiWindowFlags_NoBackground);
    widgets::sectionTitle("Mochi");
    ImGui::Text(i18n::tr("Version %s"), build::version);
    ImGui::Text(i18n::tr("Renderer: %s"), dx::api() == dx::Api::Dx12 ? "DirectX 12" : dx::api() == dx::Api::Dx11 ? "DirectX 11" : "–");

    widgets::sectionTitle("Minecraft");
    auto st = sigs::stats();
    ImGui::Text(i18n::tr("Version: %s"), st.gameVersion.c_str());
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

    auto* bg = ImGui::GetBackgroundDrawList();
    bg->AddRectFilled({0, 0}, ds, IM_COL32(10, 4, 12, int(110 * openAnim)));

    ImVec2 size{std::min(1020 * s, ds.x - 40 * s), std::min(640 * s, ds.y - 40 * s)};
    float e = draw::easeOutBack(openAnim);
    ImVec2 pos = (ds - size) * 0.5f + ImVec2(0, (1.f - e) * 24 * s);

    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, std::clamp(openAnim, 0.f, 1.f));
    ImGui::Begin("##mochi", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);

    auto* dl = ImGui::GetWindowDrawList();
    if (t.gradient) {
        draw::glow(dl, pos, pos + size, t.rounding * s, theme::col(t.accent, 0.25f), 18 * s);
    }

    float sidebar = 196 * s;
    drawSidebar(sidebar);
    ImGui::SameLine(sidebar + 34 * s);

    ImGui::BeginGroup();
    drawTopBar();
    switch (page) {
    case Page::Modules: drawGrid(); break;
    case Page::Themes: drawThemes(); break;
    case Page::Profiles: drawProfiles(); break;
    case Page::Info: drawInfo(); break;
    }
    ImGui::EndGroup();

    drawParticles(ImGui::GetForegroundDrawList());
    ImGui::End();

    if (selected && page == Page::Modules) shown = selected;
    panelAnim = draw::approach(panelAnim, selected && page == Page::Modules ? 1.f : 0.f, 16.f * t.animSpeed);
    if (panelAnim > 0.01f && shown) {
        float pw = std::min(420 * s, size.x * 0.48f);
        drawSettingsPanel({pos.x + size.x - pw - 8 * s, pos.y + 8 * s}, {pw, size.y - 16 * s});
    } else {
        shown = nullptr;
    }
    ImGui::PopStyleVar();

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
