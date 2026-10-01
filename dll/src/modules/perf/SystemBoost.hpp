#pragma once

#include "modules/Module.hpp"
#include "system/Tweaks.hpp"

class SystemBoost : public Module {
public:
    SystemBoost()
        : Module("System Boost", "Windows settings while Minecraft runs: precise timer, higher priority, no power saving. Reverted when you quit.",
                 Category::Performance, {"performance"}) {
        sub("Frame-Timing");
    }

    void onFrame() override {
        tweaks::timerResolution(timer_.b);
        tweaks::highPriority(priority_.b);
        tweaks::noPowerThrottling(power_.b);
    }

    void onDisable() override { tweaks::restore(); }

private:
    Setting& timer_ = toggleSetting("timer", "1 ms timer", true);
    Setting& priority_ = toggleSetting("priority", "Priority \"Above normal\"", true);
    Setting& power_ = toggleSetting("power", "Power saving off for Minecraft", true);
};
