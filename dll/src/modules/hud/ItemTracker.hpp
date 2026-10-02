#pragma once

#include "I18n.hpp"
#include "gui/Gui.hpp"
#include "modules/common/GameHud.hpp"
#include "modules/common/Needs.hpp"
#include "modules/common/Text.hpp"
#include "render/Ui.hpp"
#include "sdk/Game.hpp"

#include <algorithm>
#include <format>
#include <map>
#include <string>
#include <vector>

class ItemTracker : public GameList {
public:
    ItemTracker()
        : GameList("Item Tracker", "Shows what you pick up and lose for a few seconds, for example +3 Iron Ingot or -1 Ender Pearl.", need::inventory,
                   need::sigs({"LocalPlayer", "Inventory"}), {"hud-self"}, {0.845f, 0.66f}) {
        sub("Inventory info");
    }

    void onEnable() override {
        counts_.clear();
        changes_.clear();
        primed_ = false;
    }

    void onFrame() override {
        if (!game::state().inWorld) {
            primed_ = false;
            return;
        }
        auto now = totals();
        double t = ui::time();
        if (primed_) {
            for (auto& [name, n] : now) note(name, n - (counts_.count(name) ? counts_[name] : 0), t);
            for (auto& [name, n] : counts_)
                if (!now.count(name)) note(name, -n, t);
        }
        counts_ = std::move(now);
        primed_ = true;
        std::erase_if(changes_, [&](const Change& c) { return t - c.at > keep_.f; });
    }

    void onRender(ImDrawList* dl) override {
        if (changes_.empty() && !gui::editingHud()) return;
        GameList::onRender(dl);
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        if (changes_.empty()) return drawText(dl, o, s, "+3 " + std::string(i18n::tr("Iron Ingot")), ImGui::GetColorU32(gain_.color));
        double t = ui::time();
        float w = 0.f, y = 0.f;
        for (auto& c : changes_) {
            float fade = std::clamp(float(keep_.f - (t - c.at)) / 0.4f, 0.f, 1.f);
            ImVec4 col = c.delta > 0 ? gain_.color : loss_.color;
            col.w *= fade;
            auto sz = drawText(dl, o + ImVec2(0, y), s, std::format("{}{} {}", c.delta > 0 ? "+" : "", c.delta, text::pretty(c.name)), ImGui::GetColorU32(col));
            w = std::max(w, sz.x);
            y += sz.y;
        }
        return {w, y};
    }

private:
    struct Change {
        std::string name;
        int delta;
        double at;
    };

    static std::map<std::string, int> totals() {
        std::map<std::string, int> out;
        auto& p = game::state().player;
        auto add = [&](const game::Item& it) {
            if (!it.empty()) out[it.name] += it.count;
        };
        for (auto& it : p.hotbar) add(it);
        for (auto& it : p.armor) add(it);
        for (auto& it : p.main) add(it);
        add(p.offhand);
        return out;
    }

    // picking up a stack arrives over a few ticks, so changes of the same item close together are merged
    void note(const std::string& name, int delta, double t) {
        if (delta == 0) return;
        for (auto it = changes_.rbegin(); it != changes_.rend(); ++it) {
            if (it->name != name || t - it->at > 1.0) continue;
            it->delta += delta;
            it->at = t;
            if (it->delta == 0) changes_.erase(std::next(it).base());
            return;
        }
        changes_.push_back({name, delta, t});
        if (changes_.size() > size_t(lines_.i)) changes_.erase(changes_.begin());
    }

    Setting& keep_ = slider("keep", "Show for (s)", 3.f, 1.f, 10.f, "%.1f s");
    Setting& lines_ = intSlider("lines", "Lines at most", 5, 1, 12);
    Setting& gain_ = colorSetting("gain", "Picked up", {0.55f, 0.91f, 0.69f, 1.f});
    Setting& loss_ = colorSetting("loss", "Lost", {1.f, 0.4f, 0.45f, 1.f});
    std::map<std::string, int> counts_;
    std::vector<Change> changes_;
    bool primed_ = false;
};
