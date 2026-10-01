#pragma once

#include "hook/Input.hpp"
#include "modules/Module.hpp"

#include <windows.h>

class NoScroll : public Module {
public:
    NoScroll()
        : Module("Disable Mouse Wheel", "Stops you from scrolling through the hotbar by accident.", Category::Comfort,
                 {"input"}) {
        sub("Input");
    }

    void onMouse(MouseEvent& ev) override {
        if (ev.wheel == 0) return;
        if (whileSneaking_.b && !input::down(sneak_.i)) return;
        ev.cancel = true;
    }

private:
    Setting& whileSneaking_ = toggleSetting("sneak", "Only while sneaking", false);
    Setting& sneak_ = keySetting("sneakKey", "Sneak key", VK_LSHIFT);
};
