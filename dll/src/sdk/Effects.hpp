#pragma once

#include "Types.hpp"

#include <initializer_list>
#include <string>

namespace fx {

enum class Kind { Value, Flag, Skip, Out, Data, Int };

enum class Id {
    Fov,
    FovEffects,
    Gamma,
    ViewBob,
    HandBob,
    HurtCam,
    BobStrength,
    Perspective,
    SneakCam,
    Sensitivity,
    TimeOfDay,
    Rain,
    Thunder,
    Clouds,
    Sky,
    Particles,
    BlockEntities,
    Shadows,
    Fog,
    Vignette,
    FireHeight,
    Hitbox,
    HitboxColor,
    GlintColor,
    HurtColor,
    FogColor,
    WaterColor,
    ParticleScale,
    GuiScale,
    HideHand,
    HideOffhand,
    HideChat,
    HideScoreboard,
    HideCrosshair,
    HideHud,
    HandMatrix,
    LookTurn,
    LookCamera,
    SelfNametag,
    ItemPhysics,
    UseDelay,
    InventoryDelay,
    HurtAnim,
    BlockOutline,
    SwingSpeed,
    Count
};

struct Info {
    const char* sig;
    const char* label;
    Kind kind;
};

const Info& info(Id id);
std::string sig(Id id);

void begin();
void apply();
void shutdown();

void set(Id id, float v);
void scale(Id id, float m);
void add(Id id, float a);
void force(Id id, bool on);
void setInt(Id id, int v);
void skip(Id id);
void out(Id id, std::initializer_list<float> values);
void transform(Id id, game::Vec3 move, game::Vec3 scale, game::Vec3 rotateDeg);

struct Report {
    bool requested = false;
    bool installed = false;
    float value = 0.f;
};
Report report(Id id);
bool available(Id id);

}
