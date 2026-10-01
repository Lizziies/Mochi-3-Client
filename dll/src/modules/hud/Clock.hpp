#pragma once

#include "modules/HudModule.hpp"

#include <windows.h>

#include <format>

class Clock : public TextHud {
public:
    Clock() : TextHud("Clock", "Zeigt die aktuelle Uhrzeit.", {"hud-self"}, {0.01f, 0.10f}) {
        sub("Eigene Werte");
    }

protected:
    std::string value() override {
        SYSTEMTIME t;
        GetLocalTime(&t);
        std::string out;
        if (date_.b) out = std::format("{:02}.{:02}.  ", t.wDay, t.wMonth);
        int h = t.wHour;
        const char* suffix = "";
        if (twelve_.b) {
            suffix = h >= 12 ? " PM" : " AM";
            h = h % 12 == 0 ? 12 : h % 12;
        }
        if (seconds_.b) out += std::format("{:02}:{:02}:{:02}{}", h, t.wMinute, t.wSecond, suffix);
        else out += std::format("{:02}:{:02}{}", h, t.wMinute, suffix);
        return out;
    }

private:
    Setting& twelve_ = toggleSetting("12h", "12-Stunden-Format", false);
    Setting& seconds_ = toggleSetting("seconds", "Sekunden", false);
    Setting& date_ = toggleSetting("date", "Datum", false);
};
