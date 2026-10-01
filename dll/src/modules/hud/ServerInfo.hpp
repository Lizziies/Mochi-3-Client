#pragma once

#include "modules/HudModule.hpp"
#include "server/Rules.hpp"

class ServerInfo : public TextHud {
public:
    bool defaultEnabled() const override { return true; }

    ServerInfo() : TextHud("Server Display", "Zeigt, auf welchem Server du gerade bist.", {"hud-self"}, {0.01f, 0.26f}) {
        sub("Eigene Werte");
    }

    void onRender(ImDrawList* dl) override {
        if (onlyOnline_.b && rules::status().server.empty()) return;
        TextHud::onRender(dl);
    }

protected:
    std::string label() const override { return "Server"; }

    std::string value() override {
        auto st = rules::status();
        if (st.server.empty()) return "Einzelspieler";
        if (mode_.i == 1) return st.host;
        if (mode_.i == 2) return st.ip;
        return st.server;
    }

private:
    Setting& mode_ = choice("mode", "Anzeige", {"Name", "Adresse", "IP"});
    Setting& onlyOnline_ = toggleSetting("online", "Nur auf Servern anzeigen", true);
};
