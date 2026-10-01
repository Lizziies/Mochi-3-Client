#pragma once

#include "core/Config.hpp"
#include "gui/Notify.hpp"
#include "modules/Module.hpp"
#include "modules/common/Text.hpp"

#include <algorithm>
#include <sstream>

class ServerProfiles : public Module {
public:
    ServerProfiles()
        : Module("Server Profiles", "Automatically switches your settings profile when you join a server and back afterwards.", Category::Server,
                 {"cosmetic"}) {
        sub("Profiles");
    }

    void onServer(const ServerEvent& ev) override {
        if (!ev.joined) {
            if (restore_.b && !previous_.empty()) {
                config::switchProfile(previous_);
                previous_.clear();
            }
            return;
        }
        std::stringstream ss(rules_.text);
        std::string line;
        while (std::getline(ss, line, ';')) {
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string server = trim(line.substr(0, eq)), profile = trim(line.substr(eq + 1));
            if (server.empty() || profile.empty()) continue;
            if (text::lower(ev.name).find(text::lower(server)) == std::string::npos && text::lower(ev.host).find(text::lower(server)) == std::string::npos) continue;
            auto all = config::profiles();
            if (std::find(all.begin(), all.end(), profile) == all.end() || profile == config::profile()) return;
            previous_ = config::profile();
            config::switchProfile(profile);
            if (toast_.b) notify::push(i18n::tr("Profile switched"), profile + i18n::tr(" for ") + ev.name, notify::Kind::Ok);
            return;
        }
    }

private:
    static std::string trim(const std::string& s) {
        auto a = s.find_first_not_of(' '), b = s.find_last_not_of(' ');
        return a == std::string::npos ? "" : s.substr(a, b - a + 1);
    }

    Setting& rules_ = textSetting("rules", "Server=profile, separate with semicolons", "The Hive=pvp; Zeqa=pvp");
    Setting& restore_ = toggleSetting("restore", "Switch back when leaving", true);
    Setting& toast_ = toggleSetting("toast", "Show notice", true);
    std::string previous_;
};
