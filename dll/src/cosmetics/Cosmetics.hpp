#pragma once

#include <imgui.h>

#include <string>
#include <vector>

namespace cosmetics {

struct V3 {
    float x = 0.f, y = 0.f, z = 0.f;
};

struct Cube {
    V3 origin;
    V3 size;
    int u = 0;
    int v = 0;
    int tint = -1;
};

enum class Motion { None, Flap, Sway, Bob, Wag, Twitch, Float, Spin };

struct Anim {
    Motion kind = Motion::None;
    int axis = 1;
    float amplitude = 0.f;
    float speed = 1.f;
    float phase = 0.f;
};

struct Bone {
    V3 pivot;
    V3 rotation;
    Anim anim;
    std::vector<Cube> cubes;
};

struct Tint {
    std::string name;
    ImVec4 color;
};

struct Item {
    std::string id;
    std::string name;
    std::string slot;
    std::vector<Bone> bones;
    std::vector<Tint> tints;
    int texW = 0;
    int texH = 0;
    ImTextureData* texture = nullptr;
};

const std::vector<Item>& items();
void reload();
const Item* find(const std::string& id);
void drawPreview(ImDrawList* dl, ImVec2 center, float unit, float yaw, float pitch, const std::vector<const Item*>& equipped, ImVec4 body);

}
