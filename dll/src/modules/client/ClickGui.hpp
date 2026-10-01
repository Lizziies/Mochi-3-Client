#pragma once

#include "gui/Gui.hpp"
#include "modules/Module.hpp"

#include <windows.h>

class ClickGui : public Module {
public:
    ClickGui() : Module("ClickGUI", "Das Menü.", Category::Client) {
        keybind().i = VK_RSHIFT;
        keybind().label = "Menü-Taste";
    }

    bool alwaysOn() const override { return true; }

    void onKey(KeyEvent& ev) override {
        if (!ev.down || ev.repeat) return;
        int key = keybind().i ? keybind().i : VK_RSHIFT;
        if (ev.vk == key) {
            gui::toggle();
            ev.cancel = true;
        }
    }
};
