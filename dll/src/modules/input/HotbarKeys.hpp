#pragma once

#include "modules/Module.hpp"
#include "sdk/Inject.hpp"

#include <windows.h>

class HotbarKeys : public Module {
public:
    HotbarKeys()
        : Module("Hotbar Keys", "Assigns your own keys to the hotbar slots, for example mouse buttons or keys next to WASD.", Category::Comfort, {"input"}) {
        sub("Movement");
        for (int i = 0; i < 9; i++) keys_[i] = &keySetting("slot" + std::to_string(i + 1), "Slot " + std::to_string(i + 1), 0);
    }

    void onKey(KeyEvent& ev) override {
        for (int i = 0; i < 9; i++) {
            if (!keys_[i]->i || ev.vk != keys_[i]->i) continue;
            ev.cancel = true;
            if (ev.down && !ev.repeat) inject::tapLater('1' + i);
            return;
        }
    }

    void onMouse(MouseEvent& ev) override {
        int vk = ev.button == MouseButton::Middle ? VK_MBUTTON : ev.button == MouseButton::X1 ? VK_XBUTTON1 : ev.button == MouseButton::X2 ? VK_XBUTTON2 : 0;
        if (!vk) return;
        for (int i = 0; i < 9; i++) {
            if (keys_[i]->i != vk) continue;
            ev.cancel = true;
            if (ev.down) inject::tapLater('1' + i);
            return;
        }
    }

private:
    Setting* keys_[9]{};
};
