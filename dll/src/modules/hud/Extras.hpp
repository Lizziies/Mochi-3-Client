#pragma once

#include "gui/Gui.hpp"
#include "gui/Theme.hpp"
#include "modules/HudModule.hpp"
#include "modules/common/Colors.hpp"
#include "modules/common/GameHud.hpp"
#include "modules/common/Inventory.hpp"
#include "modules/common/Layout.hpp"
#include "modules/common/Needs.hpp"
#include "modules/common/Text.hpp"
#include "render/Fonts.hpp"
#include "render/Ui.hpp"
#include "sdk/Game.hpp"

#include <algorithm>
#include <cmath>
#include <format>

class HotbarArmor : public Module {
public:
    HotbarArmor()
        : Module("Hotbar Armor", "Shows your four armor pieces with durability right next to the hotbar.", Category::Hud, {"hud-self"}) {
        sub("Inventory info");
        require(need::inventory, need::sigs({"LocalPlayer", "Inventory"}));
        warnColor_.visible = [this] { return warn_.f > 0.f; };
    }

    void onRender(ImDrawList* dl) override {
        auto& st = game::state();
        if (st.player.mode == game::Mode::Spectator) return;
        auto ds = ImGui::GetIO().DisplaySize;
        float k = layout::guiScale(ds, guiScale_.f);
        float cell = 18.f * k * size_.f;
        float width = 182.f * k, top = ds.y - 22.f * k;
        float left = (ds.x - width) * 0.5f;
        float total = cell * 4.f + 4.f * k * 3.f;
        float x = side_.i == 0 ? left - total - 8.f * k + offsetX_.f : left + width + 8.f * k + offsetX_.f;
        float y = top + 2.f * k + offsetY_.f;
        for (int i = 0; i < 4; i++) {
            auto& it = st.player.armor[size_t(i)];
            if (it.empty() && hideEmpty_.b) continue;
            ImVec2 a{x + float(i) * (cell + 4.f * k), y};
            ImVec2 b = a + ImVec2(cell, cell);
            dl->AddRectFilled(a, b, IM_COL32(0, 0, 0, 110), 3.f * k);
            if (!it.empty()) {
                dl->AddRectFilled(a + ImVec2(2 * k, 2 * k), b - ImVec2(2 * k, 2 * k), materialColor(it.name), 2.f * k);
                static const char* tags[] = {"H", "C", "L", "B"};
                ImVec2 ts = fonts::bold()->CalcTextSizeA(cell * 0.55f, FLT_MAX, 0.f, tags[i]);
                dl->AddText(fonts::bold(), cell * 0.55f, a + (ImVec2(cell, cell) - ts) * 0.5f, IM_COL32(30, 20, 40, 230), tags[i]);
                if (it.enchanted) dl->AddRect(a, b, ImGui::GetColorU32(theme::current().accent), 3.f * k, 0, 1.2f * k);
            }
            if (it.maxDamage > 0) {
                float f = it.fraction();
                bool low = warn_.f > 0.f && f * 100.f < warn_.f;
                ImVec4 c = low ? warnColor_.color : rampColor(1.f - f, 0.f, 1.f, ok_.color, mid_.color, bad_.color);
                if (bar_.b) {
                    ImVec2 b0{a.x, b.y + 2.f * k};
                    dl->AddRectFilled(b0, b0 + ImVec2(cell, 3.f * k), IM_COL32(0, 0, 0, 120));
                    dl->AddRectFilled(b0, b0 + ImVec2(cell * f, 3.f * k), ImGui::GetColorU32(c));
                }
                if (percent_.b) {
                    std::string t = std::format("{:.0f}", f * 100.f);
                    ImVec2 ts = fonts::hud()->CalcTextSizeA(10.f * k * 0.9f, FLT_MAX, 0.f, t.c_str());
                    dl->AddText(fonts::hud(), 10.f * k * 0.9f, {a.x + (cell - ts.x) * 0.5f, b.y + (bar_.b ? 6.f : 2.f) * k}, ImGui::GetColorU32(c), t.c_str());
                }
            }
        }
    }

private:
    Setting& side_ = choice("side", "Side of the hotbar", {"Left", "Right"});
    Setting& size_ = slider("size", "Size", 1.f, 0.6f, 1.8f, "%.2fx");
    Setting& bar_ = toggleSetting("bar", "Durability bar", true);
    Setting& percent_ = toggleSetting("percent", "Percent", true);
    Setting& hideEmpty_ = toggleSetting("hideEmpty", "Hide empty slots", false);
    Setting& warn_ = slider("warn", "Warning below (%)", 15.f, 0.f, 50.f, "%.0f");
    Setting& offsetX_ = slider("offsetX", "Offset X", 0.f, -200.f, 200.f, "%.0f");
    Setting& offsetY_ = slider("offsetY", "Offset Y", 0.f, -200.f, 200.f, "%.0f");
    Setting& guiScale_ = slider("guiScale", "GUI scale of the game (0 = automatic)", 0.f, 0.f, 6.f, "%.0f");
    Setting& ok_ = colorSetting("ok", "Color full", {0.55f, 0.91f, 0.69f, 1.f});
    Setting& mid_ = colorSetting("mid", "Color half", {1.f, 0.82f, 0.49f, 1.f});
    Setting& bad_ = colorSetting("bad", "Color almost broken", {1.f, 0.4f, 0.45f, 1.f});
    Setting& warnColor_ = colorSetting("warnColor", "Warning color", {1.f, 0.3f, 0.35f, 1.f});
};

class FallPredictor : public GameText {
public:
    FallPredictor()
        : GameText("Fall Predictor", "Shows how far you have fallen and how much damage you would take if you landed now.", need::player, need::sigs({"LocalPlayer"}),
                   {"hud-self"}, {0.45f, 0.40f}) {
        sub("Info displays");
        warn_.visible = [this] { return colored_.b; };
    }

    void onFrame() override {
        auto& p = game::state().player;
        if (p.onGround || p.inWater || p.gliding || p.flying || p.mode == game::Mode::Creative) {
            top_ = p.pos.y;
            fall_ = 0.f;
            return;
        }
        top_ = std::max(top_, p.pos.y);
        fall_ = std::max(0.f, top_ - p.pos.y);
    }

    void onRender(ImDrawList* dl) override {
        if (fall_ < minimum_.f && !gui::editingHud()) return;
        GameText::onRender(dl);
    }

protected:
    std::string label() const override { return showLabel_.b ? i18n::tr("Fall") : ""; }

    std::string value() override {
        auto& p = game::state().player;
        float damage = std::max(0.f, std::ceil(fall_ - 3.f));
        float life = p.health + p.absorption;
        lethal_ = damage >= life && damage > 0.f;
        std::string out = i18n::fmt("{:.1f} blocks", fall_);
        if (damage > 0.f) out += i18n::fmt("  ·  {:.0f} damage", damage);
        if (lethal_ && warnText_.b) out += std::string("  ·  ") + i18n::tr("lethal");
        return out;
    }

    ImU32 valueColor() const override {
        if (!colored_.b) return textColor();
        float damage = std::max(0.f, std::ceil(fall_ - 3.f));
        if (lethal_) return ImGui::GetColorU32(lethalColor_.color);
        return ImGui::GetColorU32(damage > 0.f ? warn_.color : textColor_.color);
    }

private:
    Setting& showLabel_ = toggleSetting("label", "Label", true);
    Setting& minimum_ = slider("minimum", "Show from (blocks)", 2.f, 0.f, 10.f, "%.1f");
    Setting& colored_ = toggleSetting("colored", "Color by danger", true);
    Setting& warnText_ = toggleSetting("warnText", "Say when it would be lethal", true);
    Setting& warn_ = colorSetting("warn", "Color when it hurts", {1.f, 0.82f, 0.49f, 1.f});
    Setting& lethalColor_ = colorSetting("lethal", "Color when lethal", {1.f, 0.3f, 0.35f, 1.f});
    float top_ = 0.f;
    float fall_ = 0.f;
    mutable bool lethal_ = false;
};

class InventoryView : public GameList {
public:
    InventoryView()
        : GameList("Inventory Viewer", "Shows the contents of your inventory as a grid on the screen, with counts and durability.", need::inventory,
                   need::sigs({"LocalPlayer", "Inventory"}), {"hud-self"}, {0.01f, 0.45f}) {
        sub("Inventory info");
    }

protected:
    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        auto& p = game::state().player;
        float cell = cell_.f * s, gap = 2.f * s, y = 0.f;
        float w = 9.f * cell + 8.f * gap;
        if (title_.b) y += drawText(dl, o, s, i18n::tr("Inventory"), accentColor()).y + 3 * s;
        auto row = [&](const game::Item* items, int count, bool hotbar) {
            for (int i = 0; i < count; i++) {
                ImVec2 a = o + ImVec2(float(i) * (cell + gap), y);
                ImVec2 b = a + ImVec2(cell, cell);
                const game::Item& it = items[i];
                bool selected = hotbar && highlightHeld_.b && i == p.slot;
                dl->AddRectFilled(a, b, IM_COL32(0, 0, 0, it.empty() ? 70 : 110), 3 * s);
                if (selected) dl->AddRect(a, b, ImGui::GetColorU32(theme::current().accent), 3 * s, 0, 1.5f * s);
                if (it.empty()) continue;
                dl->AddRectFilled(a + ImVec2(2 * s, 2 * s), b - ImVec2(2 * s, 2 * s), materialColor(it.name), 2 * s);
                std::string name = text::pretty(it.name);
                if (!name.empty()) {
                    std::string initial(1, char(std::toupper((unsigned char)name[0])));
                    ImVec2 ts = textSize(s * 0.9f, initial);
                    drawText(dl, a + (ImVec2(cell, cell) - ts) * 0.5f - ImVec2(0, 2 * s), s * 0.9f, initial, IM_COL32(30, 20, 40, 220));
                }
                if (it.enchanted && glint_.b) dl->AddRect(a, b, ImGui::GetColorU32(withAlpha(theme::current().accent, 0.8f)), 3 * s, 0, 1.2f * s);
                if (it.count > 1 && counts_.b) {
                    std::string t = std::to_string(it.count);
                    ImVec2 ts = textSize(s * 0.8f, t);
                    drawText(dl, b - ts - ImVec2(2 * s, 1 * s), s * 0.8f, t, textColor());
                }
                if (it.maxDamage > 0 && durability_.b && it.damage > 0) {
                    float f = it.fraction();
                    ImVec2 b0{a.x + 2 * s, b.y - 4 * s};
                    dl->AddRectFilled(b0, b0 + ImVec2(cell - 4 * s, 2 * s), IM_COL32(0, 0, 0, 140));
                    dl->AddRectFilled(b0, b0 + ImVec2((cell - 4 * s) * f, 2 * s), ImGui::GetColorU32(rampColor(1.f - f, 0.f, 1.f, ok_.color, mid_.color, bad_.color)));
                }
            }
            y += cell + gap;
        };
        if (main_.b) {
            int rows = std::min(3, int(p.main.size() + 8) / 9);
            for (int r = 0; r < rows; r++) row(p.main.data() + r * 9, std::min(9, int(p.main.size()) - r * 9), false);
        }
        if (hotbar_.b) {
            if (main_.b) y += 3 * s;
            row(p.hotbar.data(), 9, true);
        }
        return {w, std::max(0.f, y - gap)};
    }

private:
    Setting& title_ = toggleSetting("title", "Title", false);
    Setting& cell_ = slider("cell", "Slot size", 26.f, 16.f, 48.f, "%.0f");
    Setting& main_ = toggleSetting("main", "Inventory rows", true);
    Setting& hotbar_ = toggleSetting("hotbar", "Hotbar row", true);
    Setting& highlightHeld_ = toggleSetting("held", "Mark the selected slot", true);
    Setting& counts_ = toggleSetting("counts", "Item counts", true);
    Setting& durability_ = toggleSetting("durability", "Durability bars", true);
    Setting& glint_ = toggleSetting("glint", "Outline enchanted items", true);
    Setting& ok_ = colorSetting("ok", "Color full", {0.55f, 0.91f, 0.69f, 1.f});
    Setting& mid_ = colorSetting("mid", "Color half", {1.f, 0.82f, 0.49f, 1.f});
    Setting& bad_ = colorSetting("bad", "Color almost broken", {1.f, 0.4f, 0.45f, 1.f});
};
