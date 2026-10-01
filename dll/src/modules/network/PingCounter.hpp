#pragma once

#include "Probe.hpp"
#include "modules/HudModule.hpp"
#include "modules/common/Colors.hpp"

#include <format>

class PingCounter : public TextHud {
public:
    PingCounter() : TextHud("Ping Counter", "Shows your ping to the server.", {"hud-self"}, {0.01f, 0.30f}) {
        sub("Network");
    }

    void onEnable() override { probe::use(true); }
    void onDisable() override { probe::use(false); }

protected:
    std::string label() const override { return showLabel_.b ? "Ping" : ""; }

    std::string value() override {
        auto s = probe::snapshot();
        if (!s.running || s.received == 0) return "–";
        float ms = mode_.i == 1 ? s.avg : s.last >= 0 ? s.last : s.avg;
        std::string out = std::format("{:.0f}{}", ms, unit_.b ? " ms" : "");
        if (jitter_.b) out += std::format("  ±{:.0f}", s.jitter);
        if (loss_.b && s.loss > 0.05f) out += std::format("  {:.0f}%", s.loss);
        return out;
    }

private:
    Setting& mode_ = choice("mode", "Value", {"Current", "Average"});
    Setting& unit_ = toggleSetting("unit", "Show unit", true);
    Setting& showLabel_ = toggleSetting("label", "Label", true);
    Setting& jitter_ = toggleSetting("jitter", "Append jitter", false);
    Setting& loss_ = toggleSetting("loss", "Append loss", false);
};
