#pragma once

#include "I18n.hpp"
#include "gui/Gui.hpp"
#include "modules/HudModule.hpp"
#include "server/Rules.hpp"

class ServerInfo : public TextHud {
public:
    bool defaultEnabled() const override { return true; }

    ServerInfo() : TextHud("Server Display", "Shows which server you are on.", {"hud-self"}, {0.01f, 0.26f}) {
        sub("Info displays");
    }

    void onRender(ImDrawList* dl) override {
        if (onlyOnline_.b && rules::status().server.empty() && !gui::editingHud()) return;
        TextHud::onRender(dl);
    }

protected:
    std::string label() const override { return "Server"; }

    std::string value() override {
        auto st = rules::status();
        if (st.server.empty()) return i18n::tr("Single player");
        if (mode_.i == 1) return st.host;
        if (mode_.i == 2) return st.ip;
        return st.server;
    }

private:
    Setting& mode_ = choice("mode", "Display", {"Name", "Address", "IP"});
    Setting& onlyOnline_ = toggleSetting("online", "Only show on servers", true);
};
