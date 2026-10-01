#pragma once

#include "Types.hpp"

#include <initializer_list>
#include <string>

namespace fx {

enum class Kind { Value, Flag, Skip, Out, Data, Int, Ghost, Filter };

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
    LookDelta,
    SelfNametag,
    ItemPhysics,
    UseDelay,
    InventoryDelay,
    HurtAnim,
    BlockOutline,
    SwingSpeed,
    CrystalHide,
    CrystalSimple,
    CrystalNoBase,
    GhostRender,
    GhostPick,
    CritParticle,
    HitboxEye,
    HitboxEyeColor,
    HitboxLook,
    HitboxLookColor,
    HitboxLookLength,
    HitboxWidth,
    HitboxSelf,
    HitboxJava,
    HitboxRange,
    Hitbox2D,
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
void smooth(Id id, float factor);
void out(Id id, std::initializer_list<float> values);
void transform(Id id, game::Vec3 move, game::Vec3 scale, game::Vec3 rotateDeg);

struct Report {
    bool requested = false;
    bool installed = false;
    float value = 0.f;
};
void ghost(uintptr_t actor, float seconds);
bool ghosted(uintptr_t actor);
int ghostCount();

Report report(Id id);
bool available(Id id);

}
