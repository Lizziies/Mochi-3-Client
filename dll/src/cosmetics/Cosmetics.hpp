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
    V3 uvSize;
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

struct PathPoint {
    V3 pos;
    float rx = 1.f;
    float rz = 1.f;
    float mix = 0.f;
};

struct Wave {
    int axis = 0;
    float amplitude = 0.f;
    float speed = 1.f;
    float freq = 1.f;
};

// smooth surface swept along a path: round cross section (power 2) up to squircle (power 6+)
struct Tube {
    std::vector<PathPoint> path;
    int sides = 16;
    float power = 2.f;
    int tint = -1;
    int tint2 = -1;
    bool capped = true;
    bool twoSided = false;
    Wave wave;
};

struct Bone {
    V3 pivot;
    V3 rotation;
    Anim anim;
    std::vector<Cube> cubes;
    std::vector<Tube> tubes;
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

struct Worn {
    const Item* item = nullptr;
    std::vector<ImVec4> tints;
};

struct Look {
    bool slim = false;
    float animSpeed = 1.f;
    float focus = 16.f;
};

const std::vector<Item>& items();
void reload();
const Item* find(const std::string& id);
void drawPreview(ImDrawList* dl, ImVec2 center, float unit, float yaw, float pitch, const std::vector<Worn>& worn, ImVec4 body, const Look& look = {});

}
