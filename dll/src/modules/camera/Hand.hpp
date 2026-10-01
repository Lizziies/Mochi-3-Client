#pragma once

#include "modules/Module.hpp"
#include "sdk/Effects.hpp"

class LeftHand : public Module {
public:
    LeftHand()
        : Module("Left Hand", "Moves the hand and the item in first person to the left side of the screen, mirrored.", Category::Visual, {"cosmetic"}) {
        sub("Model");
        require(0, {fx::sig(fx::Id::HandMatrix)});
    }

    void onFrame() override {
        float side = mirror_.b ? -1.f : 1.f;
        fx::transform(fx::Id::HandMatrix, {offsetX_.f, offsetY_.f, offsetZ_.f}, {side, 1.f, 1.f}, {0.f, 0.f, 0.f});
    }

private:
    Setting& mirror_ = toggleSetting("mirror", "Mirror the item", true);
    Setting& offsetX_ = slider("offsetX", "Move sideways", -1.1f, -2.f, 2.f, "%.2f");
    Setting& offsetY_ = slider("offsetY", "Move up and down", 0.f, -1.f, 1.f, "%.2f");
    Setting& offsetZ_ = slider("offsetZ", "Move forward and back", 0.f, -1.f, 1.f, "%.2f");
};
