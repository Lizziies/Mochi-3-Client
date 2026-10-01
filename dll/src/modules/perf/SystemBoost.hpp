#pragma once

#include "modules/Module.hpp"
#include "system/Tweaks.hpp"

class SystemBoost : public Module {
public:
    SystemBoost()
        : Module("System Boost", "Windows settings while Minecraft runs: precise timer, higher priority, no power saving, game scheduling for render and input threads. Reverted when you quit.",
                 Category::Performance, {"performance"}) {
        sub("Frame timing");
    }

    void onFrame() override {
        tweaks::timerResolution(timer_.b);
        tweaks::highPriority(priority_.b);
        tweaks::noPowerThrottling(power_.b);
        tweaks::inputBoost(scheduling_.b);
        tweaks::threadBoost(scheduling_.b);
    }

    void onDisable() override { tweaks::restore(); }

private:
    Setting& timer_ = toggleSetting("timer", "1 ms timer", true);
    Setting& priority_ = toggleSetting("priority", "Priority \"Above normal\"", true);
    Setting& power_ = toggleSetting("power", "Power saving off for Minecraft", true);
    Setting& scheduling_ = toggleSetting("scheduling", "Game scheduling for render and input threads", true);
};
