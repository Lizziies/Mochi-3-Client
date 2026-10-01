#pragma once

#include <imgui.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace cosmetics {

constexpr int maxCubes = 80;
constexpr int maxTexture = 256;
constexpr int maxTints = 4;

inline const std::array<const char*, 9> slots = {"head", "face", "back", "wings", "cape", "body", "waist", "shoulder", "aura"};

enum class Anim { None, Flap, Sway, Bob, Wag, Twitch, Float, Spin, Sparkle };

struct Cube {
    float origin[3]{};
    float size[3]{1, 1, 1};
    int uv[2]{};
    int tint = -1;
    int tint2 = -1;
    float mix = 0.f;
    bool flat = false;
    bool mirror = false;
};

struct Bone {
    std::string name;
    float pivot[3]{};
    float rot[3]{};
    Anim anim = Anim::None;
    char axis = 'y';
    float amp = 0.f;
    float speed = 1.f;
    float phase = 0.f;
    bool cloth = false;
    bool spring = false;
    float stiffness = 60.f;
    float damping = 7.f;
    float inertia = 1.f;
    float wind = 1.f;
    float driveAir[3]{};
    float driveSprint[3]{};
    float driveSneak[3]{};
    float driveSpeed[3]{};
    std::vector<Cube> cubes;
};

struct Tint {
    std::string name;
    uint32_t def = 0xffffff;
};

struct Item {
    std::string id;
    std::string name;
    std::string slot;
    std::vector<std::string> tags;
    std::vector<Tint> tints;
    std::vector<Bone> bones;
    ImTextureData* tex = nullptr;
    int texW = 0;
    int texH = 0;
    float texel = 1.f;
    std::filesystem::path dir;

    int cubeCount() const;
};

struct Piece {
    const Item* item = nullptr;
    std::vector<uint32_t> tint;
};

struct Motion {
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

    void step(float dt, const Motion& m);

    struct Impl;
    std::unique_ptr<Impl> d;
};

struct View {
    float yaw = 24.f;
    float pitch = -6.f;
    float zoom = 1.f;
    double time = 0.0;
    Rig* rig = nullptr;
    bool animate = true;
    bool mannequin = true;
    uint32_t skin = 0xe9a58f;
};

void tick(bool download);
void rescan();
const std::vector<std::unique_ptr<Item>>& items();
const Item* find(const std::string& id);
int loading();
std::string folder();
std::string syncStatus();

bool parse(const std::string& json, const std::filesystem::path& dir, Item& out, std::string& why);
void draw(ImDrawList* dl, ImVec2 min, ImVec2 max, const std::vector<Piece>& pieces, const View& view);

uint32_t tintOf(const Piece& piece, int index);

}
