#pragma once

#include "modules/Module.hpp"
#include "system/Tweaks.hpp"

class SystemBoost : public Module {
public:
    SystemBoost()
        : Module("System Boost", "Windows-Einstellungen, solange Minecraft läuft: genauer Timer, höhere Priorität, kein Energiesparen. "
                                 "Wird beim Beenden zurückgesetzt.",
                 Category::Performance, {"performance"}) {}

    void onFrame() override {
        tweaks::timerResolution(timer_.b);
        tweaks::highPriority(priority_.b);
        tweaks::noPowerThrottling(power_.b);
    }

    void onDisable() override { tweaks::restore(); }

private:
    Setting& timer_ = toggleSetting("timer", "1-ms-Timer", true);
    Setting& priority_ = toggleSetting("priority", "Priorität \"Höher als normal\"", true);
    Setting& power_ = toggleSetting("power", "Energiesparen für Minecraft aus", true);
};
