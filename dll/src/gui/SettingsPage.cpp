#include "GuiInternal.hpp"
#include "Gui.hpp"
#include "I18n.hpp"
#include "Theme.hpp"
#include "Widgets.hpp"
#include "core/Build.hpp"
#include "core/Config.hpp"
#include "hook/Dx.hpp"
#include "modules/Manager.hpp"
#include "modules/Tiers.hpp"
#include "modules/client/ClickGui.hpp"
#include "modules/client/ClientSettings.hpp"
#include "render/Draw.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"
#include "server/Rules.hpp"
#include "sig/Sigs.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>

namespace gui {

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

static void drawAppearance() {
    auto& t = theme::current();
    float s = ui::scale();
    ImGui::BeginChild("themes", {ImGui::GetContentRegionAvail().x, 0}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
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
    ImGui::BeginChild("profiles", {ImGui::GetContentRegionAvail().x, 0}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
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

static void drawAbout() {
    auto& t = theme::current();
    ImGui::BeginChild("info", {ImGui::GetContentRegionAvail().x, 0}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
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

    ImGui::EndChild();
}


static void settingRows(std::initializer_list<const char*> ids) {
    auto* cs = modules::get<ClientSettings>();
    if (!cs) return;
    for (auto id : ids)
        for (auto& st : cs->settings())
            if (st.id == id) widgets::setting(st);
}

static void drawGeneral() {
    float s = ui::scale();
    ImGui::BeginChild("general", {ImGui::GetContentRegionAvail().x, 0}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
    smoothScroll();
    widgets::sectionTitle("Language");
    widgets::hint("Auto follows your Windows language.");
    const char* names[] = {"Auto", "English", "Deutsch"};
    for (int i = 0; i < 3; i++) {
        if (i) ImGui::SameLine();
        if (widgets::button(names[i], {0, 0}, int(i18n::chosen()) == i)) i18n::choose(i18n::Lang(i));
    }
    ImGui::Dummy({0, 6 * s});

    widgets::sectionTitle("Menu");
    settingRows({"motion", "menuBlur", "notifications"});
    if (auto* menu = modules::get<ClickGui>()) widgets::setting(menu->keybind());
    widgets::hint("Ctrl+L: unload client · F1: hide HUD · ESC: back · right-click a key: clear");
    ImGui::EndChild();
}

static void drawChat() {
    float s = ui::scale();
    ImGui::BeginChild("chat", {ImGui::GetContentRegionAvail().x, 0}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
    smoothScroll();
    widgets::sectionTitle("Client tag");
    widgets::hint("Shown behind your own name in Better Chat and in the Tab List. Only you see it, nothing is sent to the server.");
    settingRows({"tag", "tagText", "tagColor", "tagPos", "brackets", "tabTag"});
    if (auto* chat = modules::find("Better Chat")) {
        if (!chat->available()) widgets::hint("Better Chat needs game data for this version. Until then the tag only shows in the Tab List.");
        else if (!chat->userEnabled() && widgets::button("Turn on Better Chat", {0, 0}, true)) chat->setEnabled(true);
    }
    ImGui::Dummy({0, 6 * s});
    widgets::sectionTitle("Watermark");
    settingRows({"invMark"});
    ImGui::EndChild();
}

static void applyPreset(int kind) {
    static const char* minimal[] = {"FPS", "CPS", "Keystrokes"};
    for (auto& m : modules::all()) {
        if (m->alwaysOn() || m->category() == Category::Client) continue;
        bool on = false;
        if (kind == 0) on = std::any_of(std::begin(minimal), std::end(minimal), [&](const char* n) { return m->name() == n; });
        else if (kind == 1) on = modules::tierOf(m->name()) <= 1 && !m->risky();
        m->setEnabled(on);
    }
}

static void drawModuleDefaults() {
    float s = ui::scale();
    auto& t = theme::current();
    ImGui::BeginChild("moddefaults", {ImGui::GetContentRegionAvail().x, 0}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
    smoothScroll();
    widgets::sectionTitle("Presets");
    widgets::hint("Switch many modules at once. Modules that are blocked on your server or need game data stay as they are.");
    if (widgets::button("Minimal", {0, 0}, false)) applyPreset(0);
    ImGui::SameLine();
    if (widgets::button("PvP essentials", {0, 0}, true)) applyPreset(1);
    ImGui::SameLine();
    if (widgets::button("Everything off", {0, 0}, false)) applyPreset(2);
    ImGui::Dummy({0, 6 * s});

    widgets::sectionTitle("HUD");
    settingRows({"hudScale"});
    if (widgets::button("Reset all HUD positions", {0, 0}, false))
        for (auto& m : modules::all())
            if (m->isHud()) m->resetSettings([](const Setting& st) { return st.id == "x" || st.id == "y" || st.id == "scale"; });
    ImGui::Dummy({0, 6 * s});

    widgets::sectionTitle("List");
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(i18n::tr("Show more modules"));
    ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.55f);
    widgets::toggle("more", showMoreModules());
    widgets::hint("Adds fun extras and rarely used modules to the list.");
    (void)t;
    ImGui::EndChild();
}

void drawSettingsPage(ImVec2 origin, ImVec2 size) {
    auto& t = theme::current();
    float s = ui::scale();
    static int tab = 0;
    static const char* names[] = {"General", "Chat & watermark", "Appearance", "Modules", "Profiles", "About"};
    float navW = 210 * s;
    auto* dl = ImGui::GetWindowDrawList();

    ImGui::SetCursorScreenPos(origin);
    ImGui::BeginChild("settingsnav", {navW, size.y}, 0, ImGuiWindowFlags_NoBackground);
    for (int i = 0; i < 6; i++) {
        ImGui::PushID(i);
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImVec2 sz{navW - 8 * s, 40 * s};
        bool clicked = ImGui::InvisibleButton("n", sz);
        bool hov = ImGui::IsItemHovered();
        float& a = *ImGui::GetStateStorage()->GetFloatRef(ImGui::GetItemID(), 0.f);
        a = draw::approach(a, tab == i ? 1.f : (hov ? 0.5f : 0.f), 16.f * t.animSpeed);
        auto* cdl = ImGui::GetWindowDrawList();
        if (a > 0.01f) cdl->AddRectFilled(p, p + sz, theme::col(t.surfaceHover, 0.9f * a), 10 * s);
        if (tab == i) cdl->AddRectFilled({p.x, p.y + 10 * s}, {p.x + 3 * s, p.y + sz.y - 10 * s}, theme::col(t.accent), 2 * s);
        const char* txt = i18n::tr(names[i]);
        cdl->AddText(fonts::regular(), 15.5f * s, {p.x + 16 * s, p.y + (sz.y - 15.5f * s) * 0.5f}, theme::col(theme::mix(t.textDim, t.text, std::max(a, tab == i ? 1.f : 0.f))), txt);
        if (clicked && tab != i) {
            tab = i;
            restartContentAnim();
        }
        ImGui::PopID();
        ImGui::Dummy({0, 2 * s});
    }
    ImGui::EndChild();
    dl->AddLine({origin.x + navW + 6 * s, origin.y + 6 * s}, {origin.x + navW + 6 * s, origin.y + size.y - 6 * s}, theme::col(t.surfaceHover, 0.7f), 1.f);

    ImGui::SetCursorScreenPos({origin.x + navW + 28 * s, origin.y});
    ImGui::BeginChild("settingscontent", {size.x - navW - 36 * s, size.y}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
    switch (tab) {
    case 0: drawGeneral(); break;
    case 1: drawChat(); break;
    case 2: drawAppearance(); break;
    case 3: drawModuleDefaults(); break;
    case 4: drawProfiles(); break;
    default: drawAbout(); break;
    }
    ImGui::EndChild();
}

}
