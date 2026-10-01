#pragma once

#include <imgui.h>

namespace post {

struct Params {
    float saturation = 1.f;
    float hue = 0.f;
    float brightness = 0.f;
    float contrast = 1.f;
    float gamma = 1.f;
    float sharpen = 0.f;
    float fry = 0.f;
    float flip = 0.f;
    float tint[4] = {0.f, 0.f, 0.f, 0.f};
    int tintMode = 0;
    float night[4] = {1.f, 1.f, 1.f, 0.f};
    float vignette = 0.f;
    int colorMode = 0;
    float dof = 0.f;
    float dir[2] = {0.f, 0.f};
    int dirSamples = 0;
    float blend = 0.f;

    bool active() const;
};

Params& params();
void begin();
void submit(ImDrawList* dl);
void shutdown();

}
