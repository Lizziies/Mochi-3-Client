#pragma once

#include "core/Build.hpp"
#include "modules/Module.hpp"
#include "modules/common/Text.hpp"
#include "modules/platform/Discord.hpp"
#include "render/Ui.hpp"
#include "sdk/Game.hpp"
#include "server/Rules.hpp"

#include <cstring>
#include <ctime>

class DiscordPresence : public Module {
public:
    DiscordPresence()
        : Module("Discord Rich Presence", "Shows on your Discord profile that you play Minecraft with Monchi, with server, mode and play time. You can turn off every part.",
                 Category::Client, {"cosmetic"}) {
        sub("Platform");
        stateText_.visible = [this] { return !hideAll_.b; };
        details_.visible = [this] { return !hideAll_.b; };
        showTime_.visible = [this] { return !hideAll_.b; };
        showServer_.visible = [this] { return !hideAll_.b; };
        inMenus_.visible = [this] { return !hideAll_.b; };
        smallImage_.visible = [this] { return !hideAll_.b; };
    }

    void onEnable() override {
        started_ = now();
        applied_ = "";
    }

    void onDisable() override {
        discord::stop();
        applied_ = "";
    }

    void onServer(const ServerEvent& ev) override {
        if (ev.joined) started_ = now();
    }

    void onFrame() override {
        if (appId_.text != applied_) {
            applied_ = appId_.text;
            discord::start(appId_.text);
        }
        double t = ui::time();
        if (t - lastBuild_ < 1.0) return;
        lastBuild_ = t;
        discord::set(build());
    }

    void drawSettings() override {
        ImGui::Spacing();
        const char* text = appId_.text.empty() ? i18n::tr("Enter your Discord application ID above to turn this on.") : discord::status() == discord::Status::Connected ? i18n::tr("Connected to Discord.") : i18n::tr("Looking for Discord ...");
        ImGui::TextDisabled("%s", text);
    }

private:
    static int64_t now() { return int64_t(std::time(nullptr)); }

    std::string fill(std::string pattern) const {
        auto& st = game::state();
        std::string server = st.server.empty() ? rules::status().host : st.server;
        std::string mode = st.scoreboard.title.empty() ? "" : text::strip(st.scoreboard.title);
        const std::pair<const char*, std::string> map[] = {{"{server}", server}, {"{mode}", mode}, {"{name}", nick_.b ? st.player.name : ""}, {"{version}", std::string(build::version)}};
        for (auto& [key, val] : map)
            for (size_t at = pattern.find(key); at != std::string::npos; at = pattern.find(key, at + val.size())) pattern.replace(at, std::strlen(key), val);
        while (!pattern.empty() && (pattern.back() == ' ' || pattern.back() == '-')) pattern.pop_back();
        return pattern;
    }

    discord::Presence build() const {
        discord::Presence p;
        p.active = true;
        p.largeImage = "monchi";
        p.largeText = std::string("Monchi ") + build::version;
        if (smallImage_.b) {
            p.smallImage = "heart";
            p.smallText = "Minecraft Bedrock";
        }
        p.start = showTime_.b ? started_ : 0;
        if (hideAll_.b) {
            p.details = "Playing Minecraft";
            p.start = 0;
            return p;
        }
        bool inGame = game::state().inWorld || !rules::status().server.empty();
        if (!inGame && inMenus_.b) {
            p.details = "In the menus";
            return p;
        }
        if (!inGame) {
            p.active = false;
            return p;
        }
        p.details = showServer_.b ? fill(details_.text) : "Playing Minecraft";
        p.state = fill(stateText_.text);
        return p;
    }

    Setting& appId_ = textSetting("appId", "Discord application ID", "");
    Setting& hideAll_ = toggleSetting("hideAll", "Only say that you play Minecraft", false);
    Setting& showServer_ = toggleSetting("showServer", "Show the server", true);
    Setting& details_ = textSetting("details", "First line ({server} {mode} {name} {version})", "Playing on {server}");
    Setting& stateText_ = textSetting("state", "Second line", "{mode}");
    Setting& showTime_ = toggleSetting("showTime", "Show the play time", true);
    Setting& inMenus_ = toggleSetting("inMenus", "Show a presence in the menus", true);
    Setting& smallImage_ = toggleSetting("smallImage", "Small heart icon", true);
    Setting& nick_ = toggleSetting("name", "Allow {name} to use your player name", false);
    std::string applied_;
    int64_t started_ = 0;
    double lastBuild_ = 0.0;
};
