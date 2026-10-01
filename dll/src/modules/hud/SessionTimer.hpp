#pragma once

#include "modules/HudModule.hpp"
#include "render/Ui.hpp"

#include <format>

class SessionTimer : public TextHud {
public:
    SessionTimer() : TextHud("Session Timer", "Wie lange du schon spielst – gesamt oder auf dem aktuellen Server.", {"hud-self"}, {0.01f, 0.18f}) {}

    void onServer(const ServerEvent& ev) override { serverStart_ = ev.joined ? ui::time() : -1; }

protected:
    std::string label() const override { return mode_.i == 1 ? "Server" : "Session"; }

    std::string value() override {
        double start = mode_.i == 1 ? serverStart_ : 0;
        if (start < 0) return "–";
        int total = int(ui::time() - start);
        if (total >= 3600) return std::format("{}:{:02}:{:02}", total / 3600, (total / 60) % 60, total % 60);
        return std::format("{:02}:{:02}", total / 60, total % 60);
    }

private:
    Setting& mode_ = choice("mode", "Zählt", {"Seit Spielstart", "Seit Server-Beitritt"});
    double serverStart_ = -1;
};
