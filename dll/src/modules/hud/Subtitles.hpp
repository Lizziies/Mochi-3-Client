#pragma once

#include "modules/HudModule.hpp"
#include "modules/common/Colors.hpp"
#include "modules/common/GameHud.hpp"
#include "modules/common/Needs.hpp"
#include "modules/common/Text.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"
#include "sdk/Game.hpp"

#include <algorithm>
#include <cmath>
#include <deque>

class Subtitles : public GameList {
public:
    Subtitles()
        : GameList("Subtitles", "Shows what you hear as text with a direction arrow, for example footsteps behind you or an explosion to the left.", need::player,
                   need::sigs({"LocalPlayer", "SoundEvents"}), {"hud-self"}, {0.70f, 0.78f}) {
        sub("HUD parts");
        distance_.visible = [this] { return arrows_.b; };
        fadeIn_.visible = [this] { return fade_.b; };
        fadeOut_.visible = [this] { return fade_.b; };
    }

    void onFrame() override {
        double now = ui::time();
        auto& pl = game::state().player;
        for (auto& e : game::events()) {
            if (e.kind != game::EventKind::Sound) continue;
            std::string id = e.text;
            auto skip = text::split(text::lower(ignore_.text), ',');
            bool ignored = false;
            for (auto& w : skip)
                if (text::lower(id).find(w) != std::string::npos) ignored = true;
            if (ignored) continue;
            float bearing = 0.f, dist = e.value;
            if (e.hasPos) {
                float dx = e.pos.x - pl.pos.x, dz = e.pos.z - pl.pos.z;
                dist = std::sqrt(dx * dx + dz * dz);
                float target = std::atan2(-dx, dz) * 57.2958f;
                bearing = std::fmod(target - pl.yaw + 540.f, 360.f) - 180.f;
            }
            if (maxRange_.f > 0.f && dist > maxRange_.f) continue;
            for (auto& l : list_)
                if (l.id == id) {
                    l.at = now;
                    l.bearing = bearing;
                    l.distance = dist;
                    id.clear();
                    break;
                }
            if (id.empty()) continue;
            list_.push_back({id, now, bearing, dist});
            while (int(list_.size()) > max_.i) list_.pop_front();
        }
        std::erase_if(list_, [&](const Line& l) { return now - l.at > life_.f; });
    }

    void onRender(ImDrawList* dl) override {
        if (list_.empty() && !gui::editingHud()) return;
        GameList::onRender(dl);
    }

protected:
    ImVec2 pivot() const override { return {float(anchor_.i % 3) * 0.5f, float(anchor_.i / 3) * 0.5f}; }

    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        double now = ui::time();
        float lineH = textSize(s, "Ag").y + 3 * s;
        std::vector<Line> shown(list_.begin(), list_.end());
        if (shown.empty()) shown.push_back({"random.explode", now, -60.f, 6.f});
        if (!newestFirst_.b) std::reverse(shown.begin(), shown.end());
        float w = 80.f * s, y = 0.f;
        struct Row {
            std::string text;
            float alpha;
            float bearing;
            float distance;
        };
        std::vector<Row> rows;
        for (auto& l : shown) {
            float age = float(now - l.at);
            float a = std::clamp(std::min(age / fadeIn_.f, (life_.f - age) / fadeOut_.f), 0.f, 1.f);
            std::string name = raw_.b ? l.id : title(l.id);
            if (arrows_.b && distance_.b) name += std::format("  {:.0f} m", l.distance);
            rows.push_back({name, fade_.b ? a : 1.f, l.bearing, l.distance});
            w = std::max(w, textSize(s, rows.back().text).x + (arrows_.b ? 34.f * s : 0.f));
        }
        for (auto& r : rows) {
            float arrowW = arrows_.b ? 34.f * s : 0.f;
            float textW = textSize(s, r.text).x;
            float x = (w - arrowW - textW) * textAlign();
            ImU32 col = ImGui::GetColorU32(withAlpha(textColor_.color, r.alpha));
            if (arrows_.b) {
                std::string left = r.bearing < -25.f ? "<" : "  ";
                std::string right = r.bearing > 25.f ? ">" : "  ";
                ImU32 ac = ImGui::GetColorU32(withAlpha(arrowColor_.color, r.alpha));
                drawText(dl, o + ImVec2(x, y), s, left, ac);
                drawText(dl, o + ImVec2(x + 14.f * s + textW, y), s, right, ac);
                x += 16.f * s;
            }
            drawText(dl, o + ImVec2(x, y), s, r.text, col);
            y += lineH;
        }
        return {w, y};
    }

private:
    struct Line {
        std::string id;
        double at;
        float bearing;
        float distance;
    };

    static std::string title(const std::string& id) {
        struct Pair {
            const char* key;
            const char* name;
        };
        static const Pair names[] = {{"step", "Footsteps"},      {"hurt", "Damage"},       {"explode", "Explosion"},   {"door", "Door"},       {"bow", "Bow"},
                                     {"chest", "Chest"},         {"levelup", "Level up"},  {"zombie", "Zombie"},       {"skeleton", "Skeleton"}, {"creeper", "Creeper"},
                                     {"hit", "Hit"},             {"pop", "Pickup"},        {"fire", "Fire"},           {"water", "Water"},     {"glass", "Glass breaking"},
                                     {"bell", "Bell"},           {"portal", "Portal"},     {"anvil", "Anvil"},         {"pearl", "Ender pearl"}};
        std::string low = text::lower(id);
        for (auto& p : names)
            if (low.find(p.key) != std::string::npos) return i18n::tr(p.name);
        size_t dot = id.rfind('.');
        return text::pretty(dot == std::string::npos ? id : id.substr(dot + 1));
    }

    Setting& raw_ = toggleSetting("raw", "Raw sound names", false);
    Setting& life_ = slider("life", "Visible for (s)", 3.f, 0.5f, 5.f, "%.1f s");
    Setting& max_ = intSlider("max", "Lines at most", 6, 1, 20);
    Setting& fade_ = toggleSetting("fade", "Fade in and out", true);
    Setting& fadeIn_ = slider("fadeIn", "Fade in (s)", 0.15f, 0.05f, 1.f, "%.2f");
    Setting& fadeOut_ = slider("fadeOut", "Fade out (s)", 0.6f, 0.1f, 2.f, "%.2f");
    Setting& arrows_ = toggleSetting("arrows", "Direction arrows", true);
    Setting& distance_ = toggleSetting("distance", "Distance", false);
    Setting& maxRange_ = slider("range", "Range (0 = unlimited)", 0.f, 0.f, 64.f, "%.0f");
    Setting& ignore_ = textSetting("ignore", "Ignore sounds containing (comma)", "step");
    Setting& newestFirst_ = toggleSetting("newest", "Newest on top", true);
    Setting& anchor_ = choice("anchor", "Anchor", {"Top left", "Top center", "Top right", "Middle left", "Center", "Middle right", "Bottom left", "Bottom center", "Bottom right"}, 8);
    Setting& arrowColor_ = colorSetting("arrowColor", "Arrow color", {1.f, 0.49f, 0.71f, 1.f});
    std::deque<Line> list_;
};
