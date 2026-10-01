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
#include <cmath>
#include <fstream>
#include <sstream>

namespace gui {

namespace {

int slotFilter = 0;
float yaw = 25.f;
int motion = 0;
cosmetics::Rig rig;
const char* motions[] = {"Auto", "Idle", "Walk", "Sprint", "Jump"};
const char* slots[] = {"All", "Wings", "Capes", "Head", "Face", "Back", "Body", "Feet"};
const char* slotIds[] = {"", "wings", "cape", "head", "face", "back", "body", "feet"};

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

std::string toHex(ImVec4 c) {
    char buf[16];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X", int(c.x * 255.f + 0.5f), int(c.y * 255.f + 0.5f), int(c.z * 255.f + 0.5f));
    return buf;
}

ImVec4 fromHex(const std::string& s, ImVec4 fallback) {
    if (s.size() != 7 || s[0] != '#') return fallback;
    unsigned v = (unsigned)std::strtoul(s.c_str() + 1, nullptr, 16);
    return {((v >> 16) & 255) / 255.f, ((v >> 8) & 255) / 255.f, (v & 255) / 255.f, 1.f};
}

std::vector<ImVec4> savedTints(ClientSettings* cs, const cosmetics::Item& item) {
    std::vector<ImVec4> out;
    for (auto& t : item.tints) out.push_back(t.color);
    if (!cs) return out;
    auto j = nlohmann::json::parse(cs->tints().text, nullptr, false);
    if (!j.is_object() || !j.contains(item.id) || !j[item.id].is_array()) return out;
    for (size_t i = 0; i < out.size() && i < j[item.id].size(); i++) out[i] = fromHex(j[item.id][i].get<std::string>(), out[i]);
    return out;
}

void saveTints(ClientSettings* cs, const cosmetics::Item& item, const std::vector<ImVec4>& colors) {
    if (!cs) return;
    auto j = nlohmann::json::parse(cs->tints().text, nullptr, false);
    if (!j.is_object()) j = nlohmann::json::object();
    auto arr = nlohmann::json::array();
    for (auto& c : colors) arr.push_back(toHex(c));
    j[item.id] = arr;
    cs->tints().text = j.dump();
    config::markDirty();
}

cosmetics::Moving demoMotion() {
    cosmetics::Moving m;
    int mode = motion;
    float t = float(ui::time());
    if (mode == 0) {
        float cycle = std::fmod(t, 14.f);
        mode = cycle < 3.f ? 1 : cycle < 7.f ? 2 : cycle < 11.f ? 3 : 4;
    }
    if (mode == 2) m.fwd = 4.3f;
    if (mode == 3) {
        m.fwd = 5.6f;
        m.sprint = true;
    }
    if (mode == 4) {
        float k = std::fmod(t, 1.4f);
        m.fwd = 1.5f;
        m.air = k < 0.8f;
        m.up = m.air ? 7.f * (1.f - k / 0.4f) : 0.f;
    }
    m.turn = 18.f * std::sin(t * 0.7f) * (mode == 1 ? 0.f : 1.f);
    return m;
}

struct Frame {
    float focus, zoom, yaw;
};

Frame cardFrame(const std::string& slot) {
    if (slot == "head" || slot == "face") return {30.f, 4.3f, 25.f};
    if (slot == "feet") return {3.f, 5.6f, 30.f};
    if (slot == "wings") return {17.f, 2.5f, 155.f};
    if (slot == "body" || slot == "back") return {16.f, 2.6f, 155.f};
    return {19.f, 3.5f, 155.f};
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

    rig.step(ui::dt() * (cs ? cs->animSpeed().f : 1.f), demoMotion());

    ImGui::SetCursorScreenPos(origin);
    ImGui::BeginChild("cosmetics", {listW, size.y}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
    smoothScroll();
    for (int i = 0; i < 8; i++) {
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
        auto fr = cardFrame(it.slot);
        cdl->PushClipRect(p + ImVec2(10 * s, 10 * s), p + ImVec2(w - 10 * s, 104 * s), true);
        cosmetics::drawPreview(cdl, p + ImVec2(w * 0.5f, 57 * s), fr.zoom * s, fr.yaw + std::sin(float(ui::time()) * 0.8f) * 12.f, 10.f, {{&it, savedTints(cs, it)}}, t.accent,
                               {cs ? cs->slim().b : false, fr.focus, &rig});
        cdl->PopClipRect();
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
    float stageH = size.y * 0.52f;
    dl->AddRectFilled(po, po + ImVec2(previewW, size.y), theme::col(t.surface, 0.8f), 18 * s);
    draw::glow(dl, po + ImVec2(previewW * 0.2f, stageH * 0.2f), po + ImVec2(previewW * 0.8f, stageH * 0.7f), 40 * s, theme::col(t.accent, 0.12f), 50 * s);
    ImGui::SetCursorScreenPos(po);
    ImGui::InvisibleButton("preview", {previewW, stageH});
    float spin = cs ? cs->spin().f : 18.f;
    if (ImGui::IsItemActive()) yaw -= ImGui::GetIO().MouseDelta.x * 0.7f;
    else yaw += ui::dt() * spin;
    std::vector<cosmetics::Worn> worn;
    if (cs)
        for (auto& id : split(cs->equipped().text))
            if (auto* it = cosmetics::find(id)) worn.push_back({it, savedTints(cs, *it)});
    cosmetics::Look look{cs ? cs->slim().b : false, 20.f, &rig};
    dl->PushClipRect(po, po + ImVec2(previewW, stageH), true);
    cosmetics::drawPreview(dl, {po.x + previewW * 0.5f, po.y + stageH * 0.54f}, stageH / 54.f, yaw, 12.f, worn, t.accent, look);
    dl->PopClipRect();

    ImGui::SetCursorScreenPos({po.x + 14 * s, po.y + stageH + 6 * s});
    ImGui::BeginChild("cosmeticLook", {previewW - 28 * s, size.y - stageH - 12 * s}, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse);
    smoothScroll();
    float limit = ImGui::GetWindowPos().x + ImGui::GetWindowWidth();
    for (int i = 0; i < 5; i++) {
        if (i) {
            float next = ImGui::CalcTextSize(i18n::tr(motions[i])).x + 28 * s;
            if (ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + next <= limit) ImGui::SameLine();
        }
        if (widgets::button(motions[i], {0, 0}, motion == i)) motion = i;
    }
    if (cs) {
        widgets::setting(cs->slim());
        widgets::setting(cs->spin());
        widgets::setting(cs->animSpeed());
        for (auto& w : worn) {
            ImGui::PushID(w.item->id.c_str());
            ImGui::Dummy({0, 4 * s});
            ImGui::TextUnformatted(w.item->name.c_str());
            bool changed = false;
            auto colors = w.tints;
            for (size_t i = 0; i < colors.size(); i++) {
                ImGui::SameLine(i == 0 ? ImGui::GetContentRegionAvail().x * 0.45f : 0.f);
                float c4[4] = {colors[i].x, colors[i].y, colors[i].z, 1.f};
                ImGui::PushID(int(i));
                if (ImGui::ColorEdit3("##tint", c4, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                    colors[i] = {c4[0], c4[1], c4[2], 1.f};
                    changed = true;
                }
                if (ImGui::IsItemHovered() && i < w.item->tints.size()) ImGui::SetTooltip("%s", w.item->tints[i].name.c_str());
                ImGui::PopID();
            }
            if (changed) saveTints(cs, *w.item, colors);
            ImGui::PopID();
        }
        if (worn.empty()) widgets::hint("Equip a cosmetic to change its colors here.");
    }
    ImGui::EndChild();
}

void reloadCosmetics() { cosmetics::reload(); }

}
