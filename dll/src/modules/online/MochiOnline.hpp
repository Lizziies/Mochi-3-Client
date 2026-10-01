#pragma once

#include "Online.hpp"
#include "gui/Notify.hpp"
#include "modules/Manager.hpp"
#include "modules/client/ClientSettings.hpp"
#include "cosmetics/Cosmetics.hpp"
#include "gui/Theme.hpp"
#include "modules/Module.hpp"
#include "sdk/Game.hpp"

#include <string>
#include <vector>

class MochiOnline : public Module {
public:
    MochiOnline()
        : Module("Mochi Online",
                 "Shows other Mochi users with a heart and a colored name in the Tab List and chat, and lets them see yours. Sends only your gamertag, the server name, your style and your cosmetics to the Mochi service.",
                 Category::Client, {"cosmetic"}) {
        sub("Online");
        informed_.hidden = true;
        colorB_.visible = [this] { return mode_.i == 1 || mode_.i == 3; };
        speed_.visible = [this] { return mode_.i >= 2; };
        heartColor_.visible = [this] { return heart_.b; };
        url_.visible = [this] { return !demo_.b; };
    }

    void onEnable() override {
        if (informed_.b) return;
        informed_.b = true;
        notify::push(i18n::tr("Mochi Online"), i18n::tr("Your gamertag, the server name and your style are now sent to the Mochi service. Turn this module off to stop."), notify::Kind::Info, 8.f);
    }

    void onDisable() override { push(false); }

    void onFrame() override {
        online::Style s;
        s.mode = online::Mode(mode_.i);
        s.a = rgb(colorA_.color);
        s.b = rgb(colorB_.color);
        s.speed = speed_.f;
        s.heartColor = rgb(heartColor_.color);
        s.heart = heart_.b;
        online::setStyle(s);
        online::setWorn(equipped());
        push(true);
    }

    void drawSettings() override {
        auto& t = theme::current();
        ImGui::Spacing();
        auto st = online::state();
        ImVec4 c = st == online::State::Online || st == online::State::Demo ? t.ok : st == online::State::Off ? t.textDim : t.warn;
        ImGui::TextColored(c, "%s", online::stateText().c_str());
        if (st == online::State::Online || st == online::State::Demo) ImGui::TextDisabled("%s", i18n::fmt("{} Mochi users in this list", online::count()).c_str());
        auto list = online::users();
        int shown = 0;
        for (auto& u : list) {
            if (shown++ >= 12) break;
            int idx = 0, total = 0;
            for (unsigned char ch : u.name)
                if ((ch & 0xC0) != 0x80) total++;
            for (size_t i = 0; i < u.name.size();) {
                size_t n = 1;
                unsigned char ch = (unsigned char)u.name[i];
                if (ch >= 0xF0) n = 4;
                else if (ch >= 0xE0) n = 3;
                else if (ch >= 0xC0) n = 2;
                std::string piece = u.name.substr(i, n);
                ImGui::PushStyleColor(ImGuiCol_Text, online::color(u.style, ImGui::GetTime(), idx++, total));
                ImGui::TextUnformatted(piece.c_str());
                ImGui::PopStyleColor();
                ImGui::SameLine(0, 0);
                i += n;
            }
            if (u.style.heart) {
                ImVec2 p = ImGui::GetCursorScreenPos();
                float h = ImGui::GetTextLineHeight();
                online::heartIcon(ImGui::GetWindowDrawList(), {p.x + h * 0.7f, p.y + h * 0.5f}, h * 0.8f, online::rgb(u.style.heartColor));
                ImGui::Dummy({h * 1.2f, h});
            }
            ImGui::NewLine();
        }
        ImGui::Spacing();
        if (ImGui::Button(i18n::tr("Delete my data from the service"))) {
            online::forget();
            notify::push(i18n::tr("Mochi Online"), i18n::tr("Your data will be deleted from the service."), notify::Kind::Ok);
        }
        ImGui::TextDisabled("%s", i18n::tr("The service stores your gamertag, style, cosmetics and the time you were last seen. Nothing from chat, no location, no worlds."));
    }

    bool hearts() const { return enabled() && showHearts_.b; }
    bool colors() const { return enabled() && showColors_.b; }

private:
    static uint32_t rgb(ImVec4 c) {
        return (uint32_t(c.x * 255.f + 0.5f) << 16) | (uint32_t(c.y * 255.f + 0.5f) << 8) | uint32_t(c.z * 255.f + 0.5f);
    }

    static std::vector<online::Worn> equipped() {
        std::vector<online::Worn> out;
        auto* cs = modules::get<ClientSettings>();
        if (!cs) return out;
        const std::string& list = cs->equipped().text;
        for (size_t from = 0; from <= list.size();) {
            size_t to = list.find(',', from);
            std::string id = list.substr(from, to == std::string::npos ? std::string::npos : to - from);
            if (auto* item = cosmetics::find(id)) {
                online::Worn w{item->id, {}};
                for (auto& t : item->tints) w.tint.push_back(rgb(t.color));
                out.push_back(std::move(w));
            }
            if (to == std::string::npos) break;
            from = to + 1;
        }
        return out;
    }

    void push(bool on) {
        online::Config cfg;
        cfg.on = on;
        cfg.visible = visible_.b;
        cfg.demo = on && (demo_.b || game::demo());
        cfg.url = url_.text;
        cfg.server = game::state().server;
        std::vector<std::string> names;
        for (auto& e : game::state().tab) names.push_back(e.name);
        online::tick(cfg, names, game::state().player.name);
    }

    Setting& visible_ = toggleSetting("visible", "Visible to other Mochi users", true);
    Setting& mode_ = choice("mode", "Name style", {"Solid", "Gradient", "Rainbow", "Pulse"});
    Setting& colorA_ = colorSetting("colorA", "Name color", {1.f, 0.49f, 0.71f, 1.f});
    Setting& colorB_ = colorSetting("colorB", "Second color", {1.f, 1.f, 1.f, 1.f});
    Setting& speed_ = slider("speed", "Speed", 1.f, 0.2f, 4.f, "%.1fx");
    Setting& heart_ = toggleSetting("heart", "Heart behind my name", true);
    Setting& heartColor_ = colorSetting("heartColor", "Heart color", {1.f, 0.23f, 0.36f, 1.f});
    Setting& showHearts_ = toggleSetting("showHearts", "Show hearts of other users", true);
    Setting& showColors_ = toggleSetting("showColors", "Show name colors of other users", true);
    Setting& url_ = textSetting("url", "Service address", "https://mochi-online.lisawer008.workers.dev");
    Setting& demo_ = toggleSetting("demo", "Made-up users, no network", false);
    Setting& informed_ = toggleSetting("informed", "Informed", false);
};
