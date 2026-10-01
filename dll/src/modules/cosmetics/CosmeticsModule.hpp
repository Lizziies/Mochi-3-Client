#pragma once

#include "Cosmetics.hpp"
#include "gui/Gui.hpp"
#include "gui/Theme.hpp"
#include "modules/HudModule.hpp"
#include "modules/online/Online.hpp"
#include "render/Ui.hpp"

#include <json.hpp>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

class CosmeticsModule : public HudModule {
public:
    CosmeticsModule()
        : HudModule("Cosmetics", "Wings, capes, headbands and more, free for everyone. Pick and color them here, other Mochi users see them through Mochi Online. On your own figure in the game they need a game signature that is not found yet.",
                    {"cosmetic"}, {0.84f, 0.30f}) {
        sub("Online");
        worn_.hidden = true;
        background_.b = false;
        spin_.visible = [this] { return rotate_.b; };
        angle_.visible = [this] { return !rotate_.b; };
    }

    bool alwaysOn() const override { return true; }

    void onFrame() override {
        cosmetics::tick(download_.b);
        if (!restored_ && cosmetics::loading() == 0 && !cosmetics::items().empty()) {
            restored_ = true;
            restore();
        }
        if (rotate_.b && !dragging_) yaw_ += float(ui::dt()) * spin_.f * 20.f;
        rig_.step(float(ui::dt()), motion());
        dragTurn_ = 0.f;
    }

    void onRender(ImDrawList* dl) override {
        if (!show_.b && !gui::editingHud()) return;
        HudModule::onRender(dl);
    }

    void drawSettings() override {
        auto& t = theme::current();
        ImGui::Spacing();
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 size{std::min(avail, 240.f), 260.f};
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(p, p + size, ImGui::GetColorU32(t.surface), 10.f);
        ImGui::InvisibleButton("##cosmetic_preview", size);
        dragging_ = ImGui::IsItemActive();
        if (dragging_) {
            yaw_ += ImGui::GetIO().MouseDelta.x * 0.7f;
            dragTurn_ = ImGui::GetIO().MouseDelta.x * 0.7f / std::max(0.001f, float(ui::dt()));
            pitch_ = std::clamp(pitch_ + ImGui::GetIO().MouseDelta.y * 0.3f, -35.f, 35.f);
        }
        std::vector<cosmetics::Piece> pieces = pieces_(hover_);
        draw(dl, p, p + size, pieces, yaw_);
        hover_.clear();
        ImGui::TextDisabled("%s", i18n::tr("Drag the figure to turn it. Point at a cosmetic to try it on."));
        ImGui::Spacing();

        const char* filters[] = {"All slots", "Head", "Face", "Back", "Wings", "Cape", "Body", "Waist", "Shoulder", "Aura"};
        ImGui::SetNextItemWidth(std::min(avail, 200.f));
        if (ImGui::BeginCombo("##slotfilter", i18n::tr(filters[filter_]))) {
            for (int i = 0; i < 10; i++)
                if (ImGui::Selectable(i18n::tr(filters[i]), filter_ == i)) filter_ = i;
            ImGui::EndCombo();
        }
        ImGui::BeginChild("##cosmetic_list", {0.f, 180.f}, ImGuiChildFlags_Borders);
        int shown = 0;
        for (auto& up : cosmetics::items()) {
            const auto& item = *up;
            if (filter_ > 0 && item.slot != cosmetics::slots[size_t(filter_ - 1)]) continue;
            shown++;
            bool on = worn(item.slot) == item.id;
            std::string label = item.name + "  ·  " + i18n::tr(slotLabel(item.slot));
            if (ImGui::Selectable(label.c_str(), on)) on ? takeOff(item.slot) : putOn(item);
            if (ImGui::IsItemHovered()) hover_ = item.id;
        }
        if (!shown) ImGui::TextDisabled("%s", i18n::tr(cosmetics::items().empty() ? "No cosmetics found yet." : "Nothing in this slot."));
        ImGui::EndChild();

        bool any = false;
        for (auto& slot : cosmetics::slots) {
            const cosmetics::Item* item = cosmetics::find(worn(slot));
            if (!item) continue;
            any = true;
            ImGui::TextUnformatted(item->name.c_str());
            for (size_t i = 0; i < item->tints.size(); i++) {
                Pick& pick = picks_[slot];
                pick.tint.resize(item->tints.size());
                uint32_t c = pick.tint[i] ? pick.tint[i] : item->tints[i].def;
                float col[3] = {float((c >> 16) & 255) / 255.f, float((c >> 8) & 255) / 255.f, float(c & 255) / 255.f};
                ImGui::PushID(int(i) + 100 * int(&slot - cosmetics::slots.data()));
                ImGui::SameLine();
                if (ImGui::ColorEdit3("##tint", col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                    pick.tint[i] = ((uint32_t(col[0] * 255.f + 0.5f) << 16) | (uint32_t(col[1] * 255.f + 0.5f) << 8) | uint32_t(col[2] * 255.f + 0.5f)) | 0x010101u;
                    changed();
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", item->tints[i].name.c_str());
                ImGui::PopID();
            }
        }
        if (any && ImGui::Button(i18n::tr("Take everything off"))) {
            picks_.clear();
            changed();
        }
        ImGui::Spacing();
        ImGui::TextDisabled("%s", i18n::fmt("{} cosmetics loaded", cosmetics::items().size()).c_str());
        if (cosmetics::loading()) ImGui::TextDisabled("%s", i18n::fmt("Loading {} more ...", cosmetics::loading()).c_str());
        ImGui::TextDisabled("%s", i18n::fmt("Folder: {}", cosmetics::folder()).c_str());
        ImGui::TextDisabled("%s", i18n::tr("Other Mochi users only see what you wear when Mochi Online is on."));
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        ImVec2 size{size_.f * 0.9f * s, size_.f * s};
        int count = views_.i == 1 ? 2 : 1;
        for (int i = 0; i < count; i++) {
            float yaw = rotate_.b ? yaw_ : angle_.f;
            ImVec2 at = o + ImVec2(size.x * float(i), 0.f);
            draw(dl, at, at + size, pieces_(""), yaw + 180.f * float(i));
        }
        return {size.x * float(count), size.y};
    }

private:
    struct Pick {
        std::string id;
        std::vector<uint32_t> tint;
    };

    static const char* slotLabel(const std::string& slot) {
        static const std::map<std::string, const char*> names = {{"head", "Head"}, {"face", "Face"}, {"back", "Back"}, {"wings", "Wings"}, {"cape", "Cape"},
                                                                 {"body", "Body"}, {"waist", "Waist"}, {"shoulder", "Shoulder"}, {"aura", "Aura"}};
        auto it = names.find(slot);
        return it == names.end() ? "Head" : it->second;
    }

    std::string worn(const std::string& slot) const {
        auto it = picks_.find(slot);
        return it == picks_.end() ? std::string() : it->second.id;
    }

    void putOn(const cosmetics::Item& item) {
        Pick& pick = picks_[item.slot];
        pick.id = item.id;
        pick.tint.clear();
        changed();
    }

    void takeOff(const std::string& slot) {
        picks_.erase(slot);
        changed();
    }

    std::vector<cosmetics::Piece> pieces_(const std::string& tryOn) const {
        std::vector<cosmetics::Piece> out;
        const cosmetics::Item* trial = tryOn.empty() ? nullptr : cosmetics::find(tryOn);
        for (auto& slot : cosmetics::slots) {
            cosmetics::Piece piece;
            if (trial && trial->slot == slot) {
                piece.item = trial;
            } else if (auto it = picks_.find(slot); it != picks_.end()) {
                piece.item = cosmetics::find(it->second.id);
                piece.tint = it->second.tint;
            }
            if (piece.item) out.push_back(std::move(piece));
        }
        return out;
    }

    void draw(ImDrawList* dl, ImVec2 min, ImVec2 max, const std::vector<cosmetics::Piece>& pieces, float yaw) {
        cosmetics::View v;
        v.yaw = yaw;
        v.pitch = pitch_;
        v.time = ui::time();
        v.animate = animate_.b;
        v.rig = &rig_;
        v.mannequin = figure_.b;
        cosmetics::draw(dl, min, max, pieces, v);
    }

    cosmetics::Motion motion() {
        cosmetics::Motion m;
        double t = ui::time();
        auto& st = game::state();
        int mode = motion_.i;
        if (mode == 0 && st.inWorld) {
            auto& p = st.player;
            float dt = std::max(0.001f, float(ui::dt()));
            float dyaw = p.yaw - lastYaw_;
            while (dyaw > 180.f) dyaw -= 360.f;
            while (dyaw < -180.f) dyaw += 360.f;
            lastYaw_ = p.yaw;
            m.fwd = std::hypot(p.vel.x, p.vel.z);
            m.up = p.vel.y;
            m.turn = dyaw / dt;
            m.sprint = p.sprinting;
            m.sneak = p.sneaking;
            m.air = !p.onGround;
        } else {
            int shown = mode == 0 ? 2 : mode;
            float cycle = float(std::fmod(t, shown == 3 ? 7.0 : 6.0));
            if (shown == 2 || shown == 3) {
                float top = shown == 3 ? 5.6f : 4.3f;
                float on = std::min(1.f, cycle / 0.25f) * (cycle < 3.6f ? 1.f : std::max(0.f, 1.f - (cycle - 3.6f) / 0.25f));
                m.fwd = top * on;
                m.sprint = shown == 3 && on > 0.5f;
            } else if (shown == 4) {
                float j = float(std::fmod(t, 2.4));
                m.fwd = 3.f;
                if (j < 0.6f) {
                    m.up = j < 0.3f ? 6.f : -6.f;
                    m.air = true;
                }
            }
        }
        m.turn += dragTurn_;
        return m;
    }

    void changed() {
        nlohmann::json j = nlohmann::json::object();
        for (auto& [slot, pick] : picks_) {
            nlohmann::json tint = nlohmann::json::array();
            for (auto c : pick.tint) tint.push_back(online::hex(c));
            j[slot] = {{"id", pick.id}, {"tint", tint}};
        }
        worn_.text = j.dump();
        publish();
    }

    void publish() const {
        std::vector<online::Worn> list;
        for (auto& [slot, pick] : picks_) {
            const cosmetics::Item* item = cosmetics::find(pick.id);
            online::Worn w{pick.id, {}};
            for (size_t i = 0; item && i < item->tints.size(); i++) w.tint.push_back(i < pick.tint.size() && pick.tint[i] ? pick.tint[i] : item->tints[i].def);
            list.push_back(std::move(w));
        }
        online::setWorn(list);
    }

    void restore() {
        auto j = nlohmann::json::parse(worn_.text.empty() ? "{}" : worn_.text, nullptr, false);
        if (j.is_discarded() || !j.is_object()) return;
        for (auto& [slot, v] : j.items()) {
            if (!v.is_object() || !v.contains("id") || !v["id"].is_string()) continue;
            const cosmetics::Item* item = cosmetics::find(v["id"].get<std::string>());
            if (!item || item->slot != slot) continue;
            Pick pick{item->id, {}};
            if (v.contains("tint") && v["tint"].is_array())
                for (auto& c : v["tint"]) pick.tint.push_back(c.is_string() ? online::parseHex(c.get<std::string>(), 0) : 0);
            picks_[slot] = std::move(pick);
        }
        publish();
    }

    Setting& show_ = toggleSetting("show", "Show the figure on screen", false);
    Setting& size_ = slider("size", "Size on screen", 180.f, 80.f, 420.f, "%.0f");
    Setting& figure_ = toggleSetting("figure", "Show a figure under the cosmetics", true);
    Setting& animate_ = toggleSetting("animate", "Animate cosmetics", true);
    Setting& motion_ = choice("motion", "Preview motion", {"Follow my player", "Standing", "Walking", "Running", "Jumping"});
    Setting& views_ = choice("views", "On screen", {"One angle", "Front and back"});
    Setting& rotate_ = toggleSetting("rotate", "Turn slowly", true);
    Setting& angle_ = slider("angle", "Angle", 24.f, 0.f, 360.f, "%.0f°");
    Setting& spin_ = slider("spin", "Turning speed", 1.f, 0.2f, 4.f, "%.1fx");
    Setting& download_ = toggleSetting("download", "Download new cosmetics", true);
    Setting& worn_ = textSetting("worn", "Worn", "");

    std::map<std::string, Pick> picks_;
    std::string hover_;
    int filter_ = 0;
    float yaw_ = 24.f;
    float pitch_ = -6.f;
    bool dragging_ = false;
    float dragTurn_ = 0.f;
    float lastYaw_ = 0.f;
    cosmetics::Rig rig_;
    bool restored_ = false;
};
