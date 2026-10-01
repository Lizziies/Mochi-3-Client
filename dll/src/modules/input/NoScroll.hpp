#pragma once

#include "hook/Input.hpp"
#include "modules/Module.hpp"

#include <windows.h>

class NoScroll : public Module {
public:
    NoScroll()
        : Module("Disable Mouse Wheel", "Verhindert, dass du aus Versehen durch die Hotbar scrollst.", Category::Comfort,
                 {"input"}) {
        sub("Eingabe");
    }

    void onMouse(MouseEvent& ev) override {
        if (ev.wheel == 0) return;
        if (whileSneaking_.b && !input::down(sneak_.i)) return;
        ev.cancel = true;
    }

private:
    Setting& whileSneaking_ = toggleSetting("sneak", "Nur beim Schleichen", false);
    Setting& sneak_ = keySetting("sneakKey", "Schleichen-Taste", VK_LSHIFT);
};
