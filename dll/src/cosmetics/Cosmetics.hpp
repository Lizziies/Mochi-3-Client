#pragma once

#include <imgui.h>

#include <memory>
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
    int tint2 = -1;
    float mix = 0.f;
    bool flat = false;
    bool mirror = false;
};

enum class Motion { None, Flap, Sway, Bob, Wag, Twitch, Float, Spin, Sparkle };

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

struct Physics {
    bool spring = false;
    bool cloth = false;
    float stiffness = 60.f;
    float damping = 7.f;
    float inertia = 1.f;
    float wind = 1.f;
    V3 air, sprint, sneak, speed;
};

struct Bone {
    V3 pivot;
    V3 rotation;
    Anim anim;
    Physics physics;
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
    float texel = 1.f;
    ImTextureData* texture = nullptr;
};

struct Worn {
    const Item* item = nullptr;
    std::vector<ImVec4> tints;
};

// what the wearer is doing, drives spring and cloth physics
struct Moving {
    float fwd = 0.f;
    float side = 0.f;
    float up = 0.f;
    float turn = 0.f;
    bool sprint = false;
    bool sneak = false;
    bool air = false;
};

class Rig {
public:
    Rig();
    ~Rig();
    Rig(const Rig&) = delete;
    Rig& operator=(const Rig&) = delete;
    void step(float dt, const Moving& m);
    void clear();

    struct Impl;
    std::unique_ptr<Impl> d;
};

struct Look {
    bool slim = false;
    float focus = 16.f;
    Rig* rig = nullptr;
};

const std::vector<Item>& items();
void reload();
unsigned generation();
const Item* find(const std::string& id);
void drawPreview(ImDrawList* dl, ImVec2 center, float unit, float yaw, float pitch, const std::vector<Worn>& worn, ImVec4 body, const Look& look = {});

}
