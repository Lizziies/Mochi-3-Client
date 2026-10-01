#pragma once

#include "Online.hpp"
#include "gui/Notify.hpp"
#include "gui/Theme.hpp"
#include "modules/Module.hpp"
#include "sdk/Game.hpp"

#include <string>
#include <vector>

class MochiOnline : public Module {
public:
    MochiOnline()
        : Module("Mochi Online",
                 "Shows other Mochi users with a red heart, name color and tag in the Tab List and chat, and lets them see yours. Sends only your gamertag, the server name and your style to the Mochi service.",
                 Category::Client, {"cosmetic"}) {
        sub("Online");
        informed_.hidden = true;
        colorB_.visible = [this] { return mode_.i == 1 || mode_.i == 3; };
        speed_.visible = [this] { return mode_.i >= 2; };
        tagColor_.visible = [this] { return !tag_.text.empty(); };
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
        s.tag = online::cleanTag(tag_.text);
        s.tagColor = rgb(tagColor_.color);
        s.heart = heart_.b;
        online::setStyle(s);
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
            ImVec2 p = ImGui::GetCursorScreenPos();
            float h = ImGui::GetTextLineHeight();
            if (u.style.heart) online::heartIcon(ImGui::GetWindowDrawList(), {p.x + h * 0.5f, p.y + h * 0.5f}, h * 0.8f, IM_COL32(255, 59, 92, 255));
            ImGui::Dummy({h, h});
            ImGui::SameLine();
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
            if (!u.style.tag.empty()) {
                ImGui::SameLine(0, 6);
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(online::rgb(u.style.tagColor)), "[%s]", u.style.tag.c_str());
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
    bool tags() const { return enabled() && showTags_.b; }

private:
    static uint32_t rgb(ImVec4 c) {
        return (uint32_t(c.x * 255.f + 0.5f) << 16) | (uint32_t(c.y * 255.f + 0.5f) << 8) | uint32_t(c.z * 255.f + 0.5f);
    }

    void push(bool on) {
        online::Config cfg;
        cfg.on = on;
        cfg.visible = visible_.b;
        cfg.demo = on && (demo_.b || (game::demo() && url_.text.empty()));
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
    Setting& tag_ = textSetting("tag", "Tag (max 16 characters)", "");
    Setting& tagColor_ = colorSetting("tagColor", "Tag color", {1.f, 0.49f, 0.71f, 1.f});
    Setting& heart_ = toggleSetting("heart", "Red heart before my name", true);
    Setting& showHearts_ = toggleSetting("showHearts", "Show hearts of other users", true);
    Setting& showColors_ = toggleSetting("showColors", "Show name colors of other users", true);
    Setting& showTags_ = toggleSetting("showTags", "Show tags of other users", true);
    Setting& url_ = textSetting("url", "Service address", "");
    Setting& demo_ = toggleSetting("demo", "Made-up users, no network", false);
    Setting& informed_ = toggleSetting("informed", "Informed", false);
};
