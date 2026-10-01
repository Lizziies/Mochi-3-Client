#include "GuiInternal.hpp"
#include "I18n.hpp"
#include "Theme.hpp"
#include "Widgets.hpp"
#include "core/Config.hpp"
#include "core/Paths.hpp"
#include "cosmetics/Cosmetics.hpp"
#include "modules/Manager.hpp"
#include "modules/client/ClientSettings.hpp"
#include "render/Draw.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"

#include <imgui.h>
#include <json.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>

namespace gui {

namespace {

int slotFilter = 0;
float yaw = 25.f;
const char* slots[] = {"All", "Wings", "Capes", "Head", "Back", "Body"};
const char* slotIds[] = {"", "wings", "cape", "head", "back", "body"};

std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string part;
    while (std::getline(ss, part, ',')) {
        if (!part.empty()) out.push_back(part);
    }
    return out;
}

bool isEquipped(const std::string& list, const std::string& id) {
    auto v = split(list);
    return std::find(v.begin(), v.end(), id) != v.end();
}

void toggleEquipped(Setting& st, const cosmetics::Item& item) {
    auto v = split(st.text);
    auto same = std::find(v.begin(), v.end(), item.id);
    if (same != v.end()) {
        v.erase(same);
    } else {
        std::erase_if(v, [&](const std::string& other) {
            auto* it = cosmetics::find(other);
            return it && it->slot == item.slot;
        });
        v.push_back(item.id);
    }
    std::string out;
    for (auto& e : v) out += (out.empty() ? "" : ",") + e;
    st.text = out;
    config::markDirty();
}


}

void drawCosmeticsPage(ImVec2 origin, ImVec2 size) {
    auto& t = theme::current();
    float s = ui::scale();
    auto* cs = modules::get<ClientSettings>();
    auto* dl = ImGui::GetWindowDrawList();

    float previewW = std::min(420 * s, size.x * 0.34f);
    float gap = 22 * s;
    float listW = size.x - previewW - gap;

    ImGui::SetCursorScreenPos(origin);
    ImGui::BeginChild("cosmetics", {listW, size.y}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
    smoothScroll();
    for (int i = 0; i < 6; i++) {
        if (i) ImGui::SameLine();
        if (widgets::button(slots[i], {0, 0}, slotFilter == i) && slotFilter != i) {
            slotFilter = i;
            restartContentAnim();
        }
    }
    ImGui::Dummy({0, 10 * s});

    std::vector<const cosmetics::Item*> shown;
    for (auto& it : cosmetics::items())
        if (!slotFilter || it.slot == slotIds[slotFilter]) shown.push_back(&it);

    if (shown.empty()) {
        widgets::hint(cosmetics::items().empty() ? "No cosmetics installed yet. Wings, capes and bandanas arrive with the first release and will show up here."
                                     : "Nothing in this category.");
    }

    float avail = ImGui::GetContentRegionAvail().x;
    float cg = 14 * s;
    int cols = std::max(1, int((avail + cg) / (210 * s + cg)));
    float w = (avail - cg * (cols - 1)) / cols;
    ImVec2 o = ImGui::GetCursorScreenPos();
    for (size_t i = 0; i < shown.size(); i++) {
        const cosmetics::Item& it = *shown[i];
        ImVec2 p = o + ImVec2((i % cols) * (w + cg), (i / cols) * (176 * s + cg));
        ImGui::SetCursorScreenPos(p);
        ImGui::PushID(it.id.c_str());
        bool clicked = ImGui::InvisibleButton("c", {w, 176 * s});
        bool hov = ImGui::IsItemHovered();
        ImGui::PopID();
        bool eq = cs && isEquipped(cs->equipped().text, it.id);
        auto* cdl = ImGui::GetWindowDrawList();
        float r = 16 * s;
        cdl->AddRectFilled(p, p + ImVec2(w, 176 * s), theme::col(hov ? t.surfaceHover : t.surface), r);
        if (eq) cdl->AddRect(p, p + ImVec2(w, 176 * s), theme::col(t.accent), r, 0, 2 * s);
        draw::gradientRect(cdl, p + ImVec2(10 * s, 10 * s), p + ImVec2(w - 10 * s, 104 * s), theme::col(t.accent, 0.35f), theme::col(t.accent2, 0.15f), 12 * s);
        cosmetics::drawPreview(cdl, p + ImVec2(w * 0.5f, 92 * s), 1.9f * s, 160.f, 10.f, {&it}, t.accent);
        cdl->AddText(fonts::bold(), 15.f * s, p + ImVec2(14 * s, 114 * s), theme::col(t.text), it.name.c_str());
        cdl->AddText(fonts::regular(), 12.5f * s, p + ImVec2(14 * s, 135 * s), theme::col(t.textDim), it.slot.c_str());
        const char* label = i18n::tr(eq ? "Equipped" : "Equip");
        ImVec2 ls = ImGui::CalcTextSize(label);
        ImVec2 bmin{p.x + w - ls.x - 34 * s, p.y + 138 * s}, bmax{p.x + w - 12 * s, p.y + 164 * s};
        if (eq) draw::gradientRect(cdl, bmin, bmax, theme::col(t.accent), theme::col(t.accent2), 13 * s);
        else cdl->AddRectFilled(bmin, bmax, theme::col(t.surfaceHover), 13 * s);
        cdl->AddText({(bmin.x + bmax.x) * 0.5f - ls.x * 0.5f, (bmin.y + bmax.y) * 0.5f - ls.y * 0.5f}, theme::col(eq ? t.bg : t.textDim), label);
        if (clicked && cs) toggleEquipped(cs->equipped(), it);
    }
    ImGui::EndChild();

    ImVec2 po{origin.x + listW + gap, origin.y};
    dl->AddRectFilled(po, po + ImVec2(previewW, size.y), theme::col(t.surface, 0.8f), 18 * s);
    draw::glow(dl, po + ImVec2(previewW * 0.2f, size.y * 0.25f), po + ImVec2(previewW * 0.8f, size.y * 0.6f), 40 * s, theme::col(t.accent, 0.12f), 50 * s);
    ImGui::SetCursorScreenPos(po);
    ImGui::InvisibleButton("preview", {previewW, size.y});
    if (ImGui::IsItemActive()) yaw -= ImGui::GetIO().MouseDelta.x * 0.7f;
    else yaw += ui::dt() * 18.f;
    std::vector<const cosmetics::Item*> worn;
    if (cs)
        for (auto& id : split(cs->equipped().text))
            if (auto* it = cosmetics::find(id)) worn.push_back(it);
    cosmetics::drawPreview(dl, {po.x + previewW * 0.5f, po.y + size.y * 0.55f}, 8.5f * s, yaw, 12.f, worn, t.accent);
    const char* hint = i18n::tr("Drag to rotate. Equipped cosmetics show on your character.");
    dl->AddText(fonts::regular(), 13.f * s, {po.x + 20 * s, po.y + size.y - 40 * s}, theme::col(t.textDim), hint, nullptr, previewW - 40 * s);
}

void reloadCosmetics() { cosmetics::reload(); }

}
