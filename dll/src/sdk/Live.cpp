#include "Providers.hpp"
#include "Memory.hpp"
#include "core/Log.hpp"
#include "hook/Hook.hpp"
#include "hook/Input.hpp"
#include "hook/Net.hpp"
#include "render/Ui.hpp"
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

// Fields that live behind other objects carry their pointer path as "<field>.via0", "<field>.via1", ...
// (1.26.52: position and view angles share an object at player>0x138>0x990; the copy reached through the
// ClientInstance only refreshes every few seconds).
uintptr_t follow(uintptr_t p, const char* name) {
    char key[96];
    for (int k = 0; k < 4 && p; k++) {
        std::snprintf(key, sizeof(key), "%s.via%d", name, k);
        int o = off(key);
        if (o < 0) break;
        p = mem::pointer(p + o);
    }
    return p;
}

class Live : public Provider {
public:
    Live() { self = this; }
    ~Live() override { self = nullptr; }

    unsigned supports() const override {
        unsigned m = 0;
        if (sigs::address("LocalPlayer") && off("player.posX") >= 0) m |= unsigned(Domain::Player) | unsigned(Domain::Camera);
        if (sigs::address("Level") && off("level.time") >= 0) m |= unsigned(Domain::World);
        if ((m & unsigned(Domain::Player)) && off("hit.via0") >= 0) m |= unsigned(Domain::Target) | unsigned(Domain::Combat);
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
        if (playerPtr_ && (have & unsigned(Domain::Target))) readTarget(s, ev);
        if (have & unsigned(Domain::World)) readWorld(s);
        bool playing = input::gameplay();
        // menu screens keep sending (LAN discovery, Xbox Live, server list pings), so traffic only counts
        // as "in a world" when it carried on from a moment the player was actually playing
        // a server transfer (lobby to game) drops the session for a moment, so only a longer silence counts
        bool online = net::session();
        uint64_t now = GetTickCount64();
        if (online) offlineSince_ = 0;
        else if (!offlineSince_) offlineSince_ = now;
        bool left = !online && now - offlineSince_ > 8000;
        bool grabbed = input::grabbed();
        if (grabbed) onlineSincePlay_ = online || (onlineSincePlay_ && !left);
        else if (onlineSincePlay_ && left) {
            onlineSincePlay_ = false;
            input::forgetPlay();
            playing = false;
        }
        s.inWorld = playerPtr_ != 0 || (!sigs::address("LocalPlayer") && (playing || onlineSincePlay_));
        s.screen = s.inWorld && !grabbed && !ui::wantsCursor() ? Screen::Other : Screen::None;
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
        if (o < 0) return fallback;
        uintptr_t at = follow(base, name);
        return at ? mem::get<float>(at + o, fallback) : fallback;
    }

    static int i(uintptr_t base, const char* name, int fallback = 0) {
        int o = off(name);
        if (o < 0) return fallback;
        uintptr_t at = follow(base, name);
        return at ? mem::get<int>(at + o, fallback) : fallback;
    }

    static bool b(uintptr_t base, const char* name, bool fallback = false) {
        int o = off(name);
        if (o < 0) return fallback;
        uintptr_t at = follow(base, name);
        if (!at) return fallback;
        // some states are one value of a shared enum byte ("<field>.is")
        char key[96];
        std::snprintf(key, sizeof(key), "%s.is", name);
        uint8_t v = mem::get<uint8_t>(at + o, fallback ? 1 : 0);
        int is = off(key);
        return is >= 0 ? v == is : v != 0;
    }

    void readPlayer(State& s) {
        uintptr_t p = mem::pointer(sigs::address("LocalPlayer"));
        if (!p) return;
        uintptr_t pb = follow(p, "player.posX");
        if (!pb) return;
        playerPtr_ = p;
        auto& pl = s.player;
        int px = off("player.posX");
        pl.pos = {mem::get<float>(pb + px), mem::get<float>(pb + px + 4), mem::get<float>(pb + px + 8)};
        // the game keeps the player's position at eye level, feet + 1.62 even while sneaking
        if (off("player.eyeLevel") > 0) pl.pos.y -= 1.62f;
        if (int vx = off("player.velX"); vx >= 0) {
            uintptr_t vb = follow(p, "player.velX");
            pl.vel = {mem::get<float>(vb + vx) * 20.f, mem::get<float>(vb + vx + 4) * 20.f, mem::get<float>(vb + vx + 8) * 20.f};
        } else {
            deriveVelocity(pl);
        }
        if (uintptr_t look = findLook(p, pb + px)) {
            pl.pitch = mem::get<float>(look + 12, pl.pitch);
            pl.yaw = mem::get<float>(look + 16, pl.yaw);
        }
        pl.fov = f(p, "player.fov", pl.fov);
        pl.view = View(std::clamp(i(p, "player.view", int(pl.view)), 0, 2));
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

    // The player's pick result (1.26.52: player+0x1e8): ray start = eye (3 float), ray (3 float), type (int, 0 block,
    // 1 entity, 3 nothing), face, block position (3 int), hit point (3 float), entity reference (+0x38, entity id
    // at +0x48). The game attacks whatever this points at when the attack button goes down, so a press while it
    // holds an entity is a swing at that entity.
    void readTarget(State& s, std::vector<Event>& ev) {
        uintptr_t h = follow(playerPtr_, "hit");
        auto& t = s.target;
        if (!h) {
            t.kind = Target::Kind::None;
            return;
        }
        int type = mem::get<int>(h + 0x18, 3);
        Vec3 from{mem::get<float>(h), mem::get<float>(h + 4), mem::get<float>(h + 8)};
        Vec3 at{mem::get<float>(h + 0x2c), mem::get<float>(h + 0x30), mem::get<float>(h + 0x34)};
        uintptr_t id = mem::get<uint32_t>(h + 0x48) | (uintptr_t(1) << 40);
        t.kind = type == 0 ? Target::Kind::Block : type == 1 ? Target::Kind::Entity : Target::Kind::None;
        t.pos = at;
        t.distance = t.kind == Target::Kind::None ? 0.f : distance(from, at);
        if (t.kind == Target::Kind::Block) {
            t.blockX = mem::get<int>(h + 0x20);
            t.blockY = mem::get<int>(h + 0x24);
            t.blockZ = mem::get<int>(h + 0x28);
        }

        bool press = input::down(VK_LBUTTON) && input::grabbed() && !ui::wantsCursor();
        bool edge = press && !wasPressed_;
        wasPressed_ = press;
        if (!edge || t.kind != Target::Kind::Entity) return;
        Event e{EventKind::Hit};
        e.reach = t.distance;
        e.crit = !onGround_ && fallSpeed_ < 0.f && !sprinting_;
        e.actor = id;
        e.hasPos = true;
        e.pos = at;
        ev.push_back(std::move(e));
    }

    void readWorld(State& s) {
        uintptr_t lv = mem::pointer(sigs::address("Level"));
        if (!lv) return;
        s.world.time = int(((i(lv, "level.time", s.world.time) % 24000) + 24000) % 24000);
        s.world.day = i(lv, "level.time", 0) / 24000 + 1;
        s.world.raining = f(lv, "level.rain", 0.f) > 0.05f;
        s.world.thundering = f(lv, "level.thunder", 0.f) > 0.05f;
    }

    // The view angles sit right behind a second copy of the eye position (pos, pitch, yaw), in a component the
    // player reaches through a list ("player.look.via0") whose slot changes from world to world (1.26.52: 0x990
    // in one world, 0xf20 in the next). Find the slot whose target starts with the same position, keep it while
    // it still matches.
    uintptr_t findLook(uintptr_t player, uintptr_t pos) {
        int list = off("player.look.via0");
        if (list < 0) return 0;
        auto same = [&](uintptr_t at) {
            for (int k = 0; k < 3; k++)
                if (std::fabs(mem::get<float>(at + 4 * k, 1e9f) - mem::get<float>(pos + 4 * k, -1e9f)) > 0.05f) return false;
            return true;
        };
        if (look_ && lookOwner_ == player && same(look_)) return look_;
        uint64_t now = GetTickCount64();
        if (now - lookSearched_ < 1000) return 0;
        lookSearched_ = now;
        look_ = 0;
        uintptr_t slots = mem::pointer(player + list);
        int span = off("player.look.span") > 0 ? off("player.look.span") : 0x2000;
        for (int o = 0; slots && o < span; o += 8) {
            uintptr_t c = mem::pointer(slots + o);
            if (c > 0x10000 && same(c)) {
                look_ = c;
                lookOwner_ = player;
                logger::info("live: view angles at list slot {:#x}", o);
                break;
            }
        }
        return look_;
    }

    // without a velocity field the speed comes from how far the position moved, smoothed over a few frames
    void deriveVelocity(Player& pl) {
        LARGE_INTEGER now, freq;
        QueryPerformanceCounter(&now);
        QueryPerformanceFrequency(&freq);
        double dt = lastPosQpc_ ? double(now.QuadPart - lastPosQpc_) / double(freq.QuadPart) : 0.0;
        if (dt > 0.0005 && dt < 0.5) {
            Vec3 v{float((pl.pos.x - lastPos_.x) / dt), float((pl.pos.y - lastPos_.y) / dt), float((pl.pos.z - lastPos_.z) / dt)};
            // a teleport or respawn is not movement
            if (std::fabs(v.x) > 200.f || std::fabs(v.y) > 200.f || std::fabs(v.z) > 200.f) v = {};
            float k = std::clamp(float(dt) * 12.f, 0.f, 1.f);
            vel_ = {vel_.x + (v.x - vel_.x) * k, vel_.y + (v.y - vel_.y) * k, vel_.z + (v.z - vel_.z) * k};
        }
        lastPos_ = pl.pos;
        lastPosQpc_ = now.QuadPart;
        pl.vel = vel_;
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
    Vec3 lastPos_;
    Vec3 vel_;
    int64_t lastPosQpc_ = 0;
    bool wasPressed_ = false;
    uintptr_t look_ = 0;
    uintptr_t lookOwner_ = 0;
    uint64_t lookSearched_ = 0;
    float fallSpeed_ = 0.f;
    bool onGround_ = true;
    bool sprinting_ = false;
    bool wantAttack_ = false;
    bool onlineSincePlay_ = false;
    uint64_t offlineSince_ = 0;
    bool hooked_ = false;
};

}

std::unique_ptr<Provider> makeLive() { return std::make_unique<Live>(); }

}
