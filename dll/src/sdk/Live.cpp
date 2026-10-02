#include "Providers.hpp"
#include "Memory.hpp"
#include "core/Log.hpp"
#include "hook/Hook.hpp"
#include "hook/Input.hpp"
#include "hook/Net.hpp"
#include "sig/Sigs.hpp"

#include <algorithm>
#include <cmath>
#include <mutex>

namespace game {

namespace {

class Live;
Live* self = nullptr;
bool (*originalAttack)(void*, void*) = nullptr;

int off(const char* name) { return sigs::offset(name, -1); }

class Live : public Provider {
public:
    Live() { self = this; }
    ~Live() override { self = nullptr; }

    unsigned supports() const override {
        unsigned m = 0;
        if (sigs::address("LocalPlayer") && off("player.posX") >= 0) m |= unsigned(Domain::Player) | unsigned(Domain::Camera);
        if (sigs::address("Level") && off("level.time") >= 0) m |= unsigned(Domain::World);
        if ((m & unsigned(Domain::Player)) && sigs::address("AttackEntity")) m |= unsigned(Domain::Combat);
        return m;
    }

    bool derived() const override { return true; }

    void use(unsigned mask) override {
        wantAttack_ = (mask & unsigned(Domain::Combat)) != 0;
        if (wantAttack_ && !hooked_ && (supports() & unsigned(Domain::Combat))) {
            hooked_ = hook::create("AttackEntity", reinterpret_cast<void*>(sigs::address("AttackEntity")), attack, &originalAttack);
            if (hooked_) hook::enableAll();
        }
    }

    void update(State& s, std::vector<Event>& ev) override {
        {
            std::scoped_lock g(lock_);
            for (auto& e : pending_) ev.push_back(std::move(e));
            pending_.clear();
        }
        unsigned have = supports();
        playerPtr_ = 0;
        if (have & unsigned(Domain::Player)) readPlayer(s);
        if (have & unsigned(Domain::World)) readWorld(s);
        bool playing = input::gameplay();
        // menu screens keep sending (LAN discovery, Xbox Live, server list pings), so traffic only counts
        // as "in a world" when it carried on from a moment the player was actually playing
        bool online = net::session();
        if (input::grabbed()) onlineSincePlay_ = online;
        else if (onlineSincePlay_ && !online) {
            onlineSincePlay_ = false;
            input::forgetPlay();
            playing = false;
        }
        s.inWorld = playerPtr_ != 0 || (!sigs::address("LocalPlayer") && (playing || onlineSincePlay_));
    }

    void attacked(void* actor) {
        if (!wantAttack_ || !playerPtr_) return;
        Event e{EventKind::Hit};
        e.reach = reachTo(reinterpret_cast<uintptr_t>(actor));
        e.crit = !onGround_ && fallSpeed_ < 0.f && !sprinting_;
        e.actor = reinterpret_cast<uintptr_t>(actor);
        e.crystal = isCrystal(e.actor);
        std::scoped_lock g(lock_);
        pending_.push_back(std::move(e));
    }

private:
    static bool attack(void* gm, void* actor) {
        if (self) self->attacked(actor);
        return originalAttack(gm, actor);
    }

    static float f(uintptr_t base, const char* name, float fallback = 0.f) {
        int o = off(name);
        return o < 0 ? fallback : mem::get<float>(base + o, fallback);
    }

    static int i(uintptr_t base, const char* name, int fallback = 0) {
        int o = off(name);
        return o < 0 ? fallback : mem::get<int>(base + o, fallback);
    }

    static bool b(uintptr_t base, const char* name, bool fallback = false) {
        int o = off(name);
        return o < 0 ? fallback : mem::get<uint8_t>(base + o, fallback ? 1 : 0) != 0;
    }

    void readPlayer(State& s) {
        uintptr_t p = mem::pointer(sigs::address("LocalPlayer"));
        if (!p) return;
        playerPtr_ = p;
        auto& pl = s.player;
        int px = off("player.posX");
        pl.pos = {mem::get<float>(p + px), mem::get<float>(p + px + 4), mem::get<float>(p + px + 8)};
        if (int vx = off("player.velX"); vx >= 0) pl.vel = {mem::get<float>(p + vx) * 20.f, mem::get<float>(p + vx + 4) * 20.f, mem::get<float>(p + vx + 8) * 20.f};
        pl.yaw = f(p, "player.yaw", pl.yaw);
        pl.pitch = f(p, "player.pitch", pl.pitch);
        pl.health = f(p, "player.health", pl.health);
        pl.maxHealth = f(p, "player.maxHealth", pl.maxHealth);
        pl.hunger = f(p, "player.hunger", pl.hunger);
        pl.saturation = f(p, "player.saturation", pl.saturation);
        pl.eyeHeight = f(p, "player.eyeHeight", pl.sneaking ? 1.27f : 1.62f);
        pl.air = i(p, "player.air", pl.air);
        pl.level = i(p, "player.level", pl.level);
        pl.dimension = i(p, "player.dimension", pl.dimension);
        pl.onGround = b(p, "player.onGround", pl.onGround);
        pl.sprinting = b(p, "player.sprinting", pl.sprinting);
        pl.sneaking = b(p, "player.sneaking", pl.sneaking);
        onGround_ = pl.onGround;
        sprinting_ = pl.sprinting;
        fallSpeed_ = pl.vel.y;
        eye_ = pl.eye();
    }

    void readWorld(State& s) {
        uintptr_t lv = mem::pointer(sigs::address("Level"));
        if (!lv) return;
        s.world.time = int(((i(lv, "level.time", s.world.time) % 24000) + 24000) % 24000);
        s.world.day = i(lv, "level.time", 0) / 24000 + 1;
        s.world.raining = f(lv, "level.rain", 0.f) > 0.05f;
        s.world.thundering = f(lv, "level.thunder", 0.f) > 0.05f;
    }

    static bool isCrystal(uintptr_t actor) {
        int at = off("actor.typeId"), want = off("type.crystal");
        return at >= 0 && want >= 0 && mem::get<int>(actor + at, -1) == want;
    }

    float reachTo(uintptr_t actor) const {
        int a = off("actor.aabbMinX");
        if (a >= 0) {
            float mn[3], mx[3];
            for (int k = 0; k < 3; k++) {
                mn[k] = mem::get<float>(actor + a + 4 * k);
                mx[k] = mem::get<float>(actor + a + 12 + 4 * k);
            }
            float e[3] = {eye_.x, eye_.y, eye_.z}, d2 = 0.f;
            for (int k = 0; k < 3; k++) {
                float c = std::clamp(e[k], mn[k], mx[k]);
                d2 += (c - e[k]) * (c - e[k]);
            }
            return std::sqrt(d2);
        }
        int px = off("actor.posX");
        if (px < 0) return 0.f;
        Vec3 pos{mem::get<float>(actor + px), mem::get<float>(actor + px + 4), mem::get<float>(actor + px + 8)};
        return distance(eye_, pos);
    }

    std::mutex lock_;
    std::vector<Event> pending_;
    uintptr_t playerPtr_ = 0;
    Vec3 eye_;
    float fallSpeed_ = 0.f;
    bool onGround_ = true;
    bool sprinting_ = false;
    bool wantAttack_ = false;
    bool onlineSincePlay_ = false;
    bool hooked_ = false;
};

}

std::unique_ptr<Provider> makeLive() { return std::make_unique<Live>(); }

}
