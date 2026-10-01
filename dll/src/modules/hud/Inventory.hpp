#pragma once

#include "gui/Gui.hpp"
#include "gui/Notify.hpp"
#include "gui/Theme.hpp"
#include "modules/common/Colors.hpp"
#include "modules/common/GameHud.hpp"
#include "modules/common/Inventory.hpp"
#include "modules/common/Needs.hpp"
#include "modules/common/Text.hpp"
#include "render/Draw.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"
#include "sdk/Game.hpp"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <map>

class ArmorHud : public GameList {
public:
    ArmorHud()
        : GameList("Armor HUD", "Shows your armor and the item in your hand with durability as a number, percent or bar.", need::inventory,
                   need::sigs({"LocalPlayer", "Inventory"}), {"hud-self"}, {0.01f, 0.66f}) {
        sub("Inventory info");
        flash_.visible = [this] { return warn_.f > 0.f; };
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        auto& p = game::state().player;
        std::vector<std::pair<const game::Item*, const char*>> items;
        static const char* names[] = {"K", "B", "H", "S"};
        for (int i = 0; i < 4; i++) items.push_back({&p.armor[size_t(i)], names[i]});
        if (held_.b) items.push_back({&p.held(), "R"});
        if (offhand_.b) items.push_back({&p.offhand, "N"});

        bool horizontal = layout_.i == 1;
        float size = iconSize_.f * s, gap = 4 * s, x = 0.f, y = 0.f, w = 0.f, h = 0.f;
        double now = ui::time();
        for (auto& [it, tag] : items) {
            if (it->empty() && hideEmpty_.b) continue;
            ImVec2 a = o + ImVec2(x, y);
            dl->AddRectFilled(a, a + ImVec2(size, size), materialColor(it->name), 5 * s);
            if (it->empty()) dl->AddRectFilled(a, a + ImVec2(size, size), IM_COL32(0, 0, 0, 120), 5 * s);
            ImVec2 ts = textSize(s * 0.8f, tag);
            drawText(dl, a + (ImVec2(size, size) - ts) * 0.5f, s * 0.8f, tag, IM_COL32(30, 20, 40, 230));
            if (it->enchanted && glint_.b) dl->AddRect(a, a + ImVec2(size, size), ImGui::GetColorU32(theme::current().accent), 5 * s, 0, 1.5f * s);

            float tx = a.x + size + 6 * s, rowW = size;
            if (it->maxDamage > 0) {
                float f = it->fraction();
                bool low = warn_.f > 0.f && f * 100.f < warn_.f;
                ImVec4 c = rampColor(1.f - f, 0.f, 1.f, ok_.color, mid_.color, bad_.color);
                if (low && flash_.b && std::fmod(now, 0.8) < 0.4) c = bad_.color;
                std::string t;
                if (mode_.i == 0 || mode_.i == 3) t = std::to_string(it->left());
                if (mode_.i == 1 || mode_.i == 3) t += std::format("{}{:.0f}%", t.empty() ? "" : "  ", f * 100.f);
                float tw = 0.f;
                if (!t.empty()) tw = drawText(dl, {tx, a.y + (size - fonts::hudSize() * s) * 0.5f - (mode_.i == 2 ? 4 * s : 0)}, s, t, ImGui::GetColorU32(c)).x;
                if (mode_.i == 2 || bar_.b) {
                    float bw = std::max(48.f * s, tw);
                    ImVec2 b0{tx, a.y + size - 6 * s};
                    dl->AddRectFilled(b0, b0 + ImVec2(bw, 4 * s), IM_COL32(0, 0, 0, 90), 2 * s);
                    dl->AddRectFilled(b0, b0 + ImVec2(bw * f, 4 * s), ImGui::GetColorU32(c), 2 * s);
                    tw = std::max(tw, bw);
                }
                rowW = size + 6 * s + tw;
            } else if (!it->empty() && it->count > 1) {
                rowW = size + 6 * s + drawText(dl, {tx, a.y + (size - fonts::hudSize() * s) * 0.5f}, s, std::format("x{}", it->count), textColor()).x;
            }
            if (horizontal) {
                x += rowW + gap * 2;
                w = x;
                h = size;
            } else {
                y += size + gap;
                w = std::max(w, rowW);
                h = y;
            }
        }
        return {std::max(w, size), std::max(h, size)};
    }

private:
    Setting& layout_ = choice("layout", "Layout", {"Stacked", "Side by side"});
    Setting& mode_ = choice("mode", "Durability", {"Number", "Percent", "Bar", "Number and percent"}, 1);
    Setting& bar_ = toggleSetting("bar", "Also show a bar", false);
    Setting& iconSize_ = slider("icon", "Icon size", 20.f, 12.f, 40.f, "%.0f");
    Setting& held_ = toggleSetting("held", "Item in hand", true);
    Setting& offhand_ = toggleSetting("offhand", "Offhand", false);
    Setting& hideEmpty_ = toggleSetting("hideEmpty", "Hide empty slots", true);
    Setting& glint_ = toggleSetting("glint", "Outline enchanted", true);
    Setting& warn_ = slider("warn", "Warn below (%)", 15.f, 0.f, 50.f, "%.0f%%");
    Setting& flash_ = toggleSetting("flash", "Flash on warning", true);
    Setting& ok_ = colorSetting("ok", "Color full", {0.55f, 0.91f, 0.69f, 1.f});
    Setting& mid_ = colorSetting("mid", "Color half", {1.f, 0.82f, 0.49f, 1.f});
    Setting& bad_ = colorSetting("bad", "Color almost broken", {1.f, 0.4f, 0.45f, 1.f});
};

class PotionHud : public GameList {
public:
    PotionHud()
        : GameList("Potion HUD", "Shows your active effects with time left, sorted and colored. Flashes before an effect runs out.", need::effects,
                   need::sigs({"LocalPlayer", "Effects"}), {"hud-self"}, {0.85f, 0.10f}) {
        sub("Inventory info");
        blink_.visible = [this] { return blinkAt_.f > 0.f; };
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        std::vector<game::Effect> list = game::state().player.effects;
        std::erase_if(list, [&](const game::Effect& e) { return (e.good && !showGood_.b) || (!e.good && !showBad_.b); });
        auto key = [&](const game::Effect& e) { return sort_.i == 0 ? -e.seconds : sort_.i == 1 ? e.seconds : 0.f; };
        std::sort(list.begin(), list.end(), [&](auto& a, auto& b) { return sort_.i == 2 ? a.id < b.id : key(a) < key(b); });
        if ((int)list.size() > max_.i) list.resize(size_t(max_.i));

        float y = 0.f, w = 100.f * s;
        double now = ui::time();
        for (auto& e : list) {
            std::string name = text::effect(e.id) + (e.amplifier > 0 ? " " + text::roman(e.amplifier + 1) : "");
            ImU32 col = textColor();
            if (colored_.b) col = IM_COL32((e.color >> 16) & 255, (e.color >> 8) & 255, e.color & 255, 255);
            if (blinkAt_.f > 0.f && e.seconds < blinkAt_.f && std::fmod(now, 0.6) < 0.3) col = ImGui::GetColorU32(blink_.color);
            float x = 0.f;
            if (swatch_.b) {
                dl->AddRectFilled(o + ImVec2(0, y + 2 * s), o + ImVec2(8 * s, y + 10 * s), IM_COL32((e.color >> 16) & 255, (e.color >> 8) & 255, e.color & 255, 255), 2 * s);
                x = 12 * s;
            }
            auto a = drawText(dl, o + ImVec2(x, y), s, name, col);
            std::string t = text::clock(e.seconds);
            auto b = textSize(s, t);
            float rowW = x + a.x + 14 * s + b.x;
            w = std::max(w, rowW);
            drawText(dl, o + ImVec2(std::max(x + a.x + 14 * s, w - b.x), y), s, t, col);
            y += a.y;
            if (bar_.b && e.total > 0.f) {
                ImVec2 b0 = o + ImVec2(x, y);
                dl->AddRectFilled(b0, b0 + ImVec2(w - x, 3 * s), IM_COL32(0, 0, 0, 80), 1.5f * s);
                dl->AddRectFilled(b0, b0 + ImVec2((w - x) * std::clamp(e.seconds / e.total, 0.f, 1.f), 3 * s), col, 1.5f * s);
                y += 5 * s;
            }
        }
        if (list.empty()) return drawText(dl, o, s, i18n::tr("No effects"), ImGui::GetColorU32(theme::current().textDim));
        return {w, y};
    }

private:
    Setting& sort_ = choice("sort", "Sorting", {"Longest first", "Shortest first", "By name"});
    Setting& max_ = intSlider("max", "Show at most", 8, 1, 16);
    Setting& showGood_ = toggleSetting("good", "Positive effects", true);
    Setting& showBad_ = toggleSetting("bad", "Negative effects", true);
    Setting& colored_ = toggleSetting("colored", "Color by effect", true);
    Setting& swatch_ = toggleSetting("swatch", "Color swatch", true);
    Setting& bar_ = toggleSetting("bar", "Time-left bar", true);
    Setting& blinkAt_ = slider("blinkAt", "Flash below (s)", 10.f, 0.f, 60.f, "%.0f s");
    Setting& blink_ = colorSetting("blink", "Flash color", {1.f, 0.4f, 0.45f, 1.f});
};

class CountHud : public GameText {
public:
    CountHud(std::string name, std::string desc, std::string item, int aux, ImVec2 pos)
        : GameText(std::move(name), std::move(desc), need::inventory, need::sigs({"LocalPlayer", "Inventory"}), {"hud-self"}, pos), item_(std::move(item)),
          aux_(aux) {
        sub("Inventory info");
    }

protected:
    virtual std::string itemName() const { return item_; }
    virtual int itemAux() const { return aux_; }

    std::string label() const override { return showLabel_.b ? title() : ""; }

    std::string value() override {
        int n = countItems(itemName(), itemAux(), hotbarOnly_.b);
        shown_ = n;
        return std::to_string(n);
    }

    ImU32 valueColor() const override {
        if (lowAt_.i > 0 && shown_ <= lowAt_.i) return ImGui::GetColorU32(lowColor_.color);
        return textColor();
    }

    virtual std::string title() const { return text::pretty(itemName()); }

private:
    Setting& showLabel_ = toggleSetting("label", "Label", true);
    Setting& hotbarOnly_ = toggleSetting("hotbarOnly", "Count hotbar only", false);
    Setting& lowAt_ = intSlider("lowAt", "Warning color at most (0 = off)", 0, 0, 64);
    Setting& lowColor_ = colorSetting("lowColor", "Warning color", {1.f, 0.4f, 0.45f, 1.f});
    std::string item_;
    int aux_;
    mutable int shown_ = 0;
};

class PotCounter : public CountHud {
public:
    PotCounter() : CountHud("Pot Counter", "Counts your healing (splash) potions in the inventory.", "splash_potion", 21, {0.01f, 0.70f}) {}

protected:
    std::string itemName() const override { return name_.text; }
    int itemAux() const override { return aux_.i; }
    std::string title() const override { return "Pots"; }

private:
    Setting& name_ = textSetting("item", "Item name", "splash_potion");
    Setting& aux_ = intSlider("aux", "Potion value (-1 = any)", 21, -1, 120);
};

class ArrowCounter : public CountHud {
public:
    ArrowCounter() : CountHud("Arrow Counter", "Counts your arrows.", "arrow", -1, {0.01f, 0.74f}) {}

protected:
    std::string title() const override { return i18n::tr("Arrows"); }
};

class TotemCounter : public CountHud {
public:
    TotemCounter() : CountHud("Totem Counter", "Counts your totems of undying.", "totem_of_undying", -1, {0.01f, 0.78f}) {}

protected:
    std::string title() const override { return "Totems"; }
};

class ItemCounter : public CountHud {
public:
    ItemCounter() : CountHud("Item Counter", "Counts any item that you choose.", "golden_apple", -1, {0.01f, 0.82f}) {}

protected:
    std::string itemName() const override { return name_.text; }
    int itemAux() const override { return aux_.i; }
    std::string title() const override { return text::pretty(name_.text); }

private:
    Setting& name_ = textSetting("item", "Item name (e.g. golden_apple)", "golden_apple");
    Setting& aux_ = intSlider("aux", "Variant (-1 = any)", -1, -1, 120);
};

class DurabilityWarning : public Module {
public:
    DurabilityWarning()
        : Module("Durability Warning", "Warns in the middle of the screen when armor or tools are almost broken, with an optional sound.", Category::Hud,
                 {"hud-self"}) {
        sub("Inventory info");
        require(need::inventory, need::sigs({"LocalPlayer", "Inventory"}));
    }

    void onFrame() override {
        auto& p = game::state().player;
        std::string found;
        auto check = [&](const game::Item& it) {
            if (it.empty() || it.maxDamage <= 0) return;
            if (it.fraction() * 100.f < threshold_.f && (found.empty() || it.fraction() < worst_)) {
                found = text::pretty(it.name);
                worst_ = it.fraction();
            }
        };
        worst_ = 1.f;
        if (armor_.b)
            for (auto& it : p.armor) check(it);
        if (held_.b) check(p.held());
        if (found != current_) {
            current_ = found;
            if (!found.empty()) {
                shownAt_ = ui::time();
                if (sound_.b) MessageBeep(MB_ICONEXCLAMATION);
            }
        }
    }

    void onRender(ImDrawList* dl) override {
        if (current_.empty()) return;
        double age = ui::time() - shownAt_;
        if (hold_.f > 0.f && age > hold_.f) return;
        auto ds = ImGui::GetIO().DisplaySize;
        float pulse = pulse_.b ? 0.65f + 0.35f * std::sin(float(age) * 6.f) : 1.f;
        ImVec4 c = color_.color;
        c.w *= pulse;
        std::string t = i18n::fmt("{} almost broken ({:.0f}%)", current_, worst_ * 100.f);
        draw::textCentered(dl, fonts::bold(), size_.f * ui::scale(), {ds.x * 0.5f, ds.y * y_.f}, ImGui::GetColorU32(c), t.c_str());
    }

private:
    Setting& threshold_ = slider("threshold", "Warn below (%)", 10.f, 1.f, 50.f, "%.0f%%");
    Setting& armor_ = toggleSetting("armor", "Check armor", true);
    Setting& held_ = toggleSetting("held", "Check item in hand", true);
    Setting& sound_ = toggleSetting("sound", "Sound", false);
    Setting& pulse_ = toggleSetting("pulse", "Pulse", true);
    Setting& hold_ = slider("hold", "Display time (s, 0 = permanent)", 6.f, 0.f, 30.f, "%.0f s");
    Setting& size_ = slider("size", "Size", 24.f, 12.f, 56.f, "%.0f");
    Setting& y_ = slider("y", "Height (fraction)", 0.25f, 0.05f, 0.9f, "%.2f");
    Setting& color_ = colorSetting("color", "Color", {1.f, 0.4f, 0.45f, 1.f});
    std::string current_;
    float worst_ = 1.f;
    double shownAt_ = -100.0;
};

class LowHealth : public Module {
public:
    LowHealth()
        : Module("Low Health Indicator", "A red, pulsing screen edge when your health is low.", Category::Hud, {"hud-self"}) {
        sub("Own values");
        require(need::player, need::sigs({"LocalPlayer"}));
    }

    void onRender(ImDrawList* dl) override {
        auto& p = game::state().player;
        float low = threshold_.f * 2.f;
        float target = p.health > 0.f && p.health <= low ? 1.f - p.health / low : 0.f;
        level_ = draw::approach(level_, target, 6.f);
        if (level_ < 0.01f) return;
        double t = ui::time();
        float beat = pulse_.b ? 0.75f + 0.25f * std::sin(float(t) * (4.f + 6.f * level_)) : 1.f;
        float a = std::clamp(intensity_.f * level_ * beat, 0.f, 1.f);
        auto ds = ImGui::GetIO().DisplaySize;
        float e = std::min(ds.x, ds.y) * size_.f;
        ImU32 solid = ImGui::GetColorU32(withAlpha(color_.color, a));
        ImU32 clear = ImGui::GetColorU32(withAlpha(color_.color, 0.f));
        dl->AddRectFilledMultiColor({0, 0}, {ds.x, e}, solid, solid, clear, clear);
        dl->AddRectFilledMultiColor({0, ds.y - e}, {ds.x, ds.y}, clear, clear, solid, solid);
        dl->AddRectFilledMultiColor({0, 0}, {e, ds.y}, solid, clear, clear, solid);
        dl->AddRectFilledMultiColor({ds.x - e, 0}, {ds.x, ds.y}, clear, solid, solid, clear);
    }

private:
    Setting& threshold_ = slider("threshold", "From (hearts)", 4.f, 1.f, 10.f, "%.1f ♥");
    Setting& intensity_ = slider("intensity", "Strength", 0.55f, 0.1f, 1.f, "%.2f");
    Setting& size_ = slider("size", "Edge width", 0.18f, 0.05f, 0.4f, "%.2f");
    Setting& pulse_ = toggleSetting("pulse", "Heartbeat pulse", true);
    Setting& color_ = colorSetting("color", "Color", {1.f, 0.1f, 0.2f, 1.f});
    float level_ = 0.f;
};

class BetterHunger : public GameList {
public:
    BetterHunger()
        : GameList("Better Hunger Bar", "Shows hunger and saturation as a bar and previews what the food in your hand gives.", need::player | game::Domain::Inventory,
                   need::sigs({"LocalPlayer", "Inventory"}), {"hud-self"}, {0.01f, 0.86f}) {
        sub("Own values");
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        auto& p = game::state().player;
        float w = width_.f * s, h = 9 * s;
        ImVec2 b0 = o;
        dl->AddRectFilled(b0, b0 + ImVec2(w, h), IM_COL32(0, 0, 0, 90), h * 0.5f);
        float hunger = std::clamp(p.hunger / 20.f, 0.f, 1.f);
        dl->AddRectFilled(b0, b0 + ImVec2(w * hunger, h), ImGui::GetColorU32(hungerColor_.color), h * 0.5f);
        float gain = 0.f, satGain = 0.f;
        bool food = foodValue(p.held().name, gain, satGain);
        if (preview_.b && food) {
            float to = std::clamp((p.hunger + gain) / 20.f, 0.f, 1.f);
            dl->AddRectFilled(b0 + ImVec2(w * hunger, 0), b0 + ImVec2(w * to, h), ImGui::GetColorU32(withAlpha(hungerColor_.color, 0.4f)), h * 0.5f);
        }
        float sat = std::clamp(p.saturation / 20.f, 0.f, 1.f);
        dl->AddRectFilled(b0 + ImVec2(0, h - 3 * s), b0 + ImVec2(w * std::min(sat, hunger), h), ImGui::GetColorU32(satColor_.color), 1.5f * s);
        float y = h + 3 * s;
        if (numbers_.b) {
            std::string t = i18n::fmt("Hunger {:.0f}  ·  Saturation {:.1f}", p.hunger, p.saturation);
            if (preview_.b && food) t += std::format("  ·  +{:.0f} / +{:.1f}", gain, satGain);
            y += drawText(dl, o + ImVec2(0, y), s, t, textColor()).y;
        }
        return {std::max(w, 140.f * s), y};
    }

private:
    static bool foodValue(const std::string& n, float& gain, float& sat) {
        struct F {
            const char* name;
            float gain;
            float sat;
        };
        static const F foods[] = {
            {"apple", 4, 2.4f},          {"golden_apple", 4, 9.6f},    {"enchanted_golden_apple", 4, 9.6f}, {"bread", 5, 6.f},
            {"cooked_beef", 8, 12.8f},   {"cooked_porkchop", 8, 12.8f}, {"cooked_chicken", 6, 7.2f},       {"cooked_mutton", 6, 9.6f},
            {"cooked_cod", 5, 6.f},      {"cooked_salmon", 6, 9.6f},   {"cooked_rabbit", 5, 6.f},          {"baked_potato", 5, 6.f},
            {"carrot", 3, 3.6f},         {"golden_carrot", 6, 14.4f},  {"melon_slice", 2, 1.2f},           {"cookie", 2, 0.4f},
            {"pumpkin_pie", 8, 4.8f},    {"mushroom_stew", 6, 7.2f},   {"rabbit_stew", 10, 12.f},          {"beetroot_soup", 6, 7.2f},
            {"sweet_berries", 2, 0.4f},  {"dried_kelp", 1, 0.6f},      {"beef", 3, 1.8f},                  {"porkchop", 3, 1.8f},
        };
        for (auto& f : foods)
            if (n == f.name) {
                gain = f.gain;
                sat = f.sat;
                return true;
            }
        return false;
    }

    Setting& width_ = slider("width", "Width", 140.f, 60.f, 300.f, "%.0f");
    Setting& numbers_ = toggleSetting("numbers", "Numbers", true);
    Setting& preview_ = toggleSetting("preview", "Preview for food in hand", true);
    Setting& hungerColor_ = colorSetting("hunger", "Hunger color", {0.85f, 0.6f, 0.3f, 1.f});
    Setting& satColor_ = colorSetting("sat", "Saturation color", {1.f, 0.9f, 0.4f, 1.f});
};
