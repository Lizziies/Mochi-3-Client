#include "Providers.hpp"
#include "Memory.hpp"
#include "core/Bg.hpp"
#include "core/Client.hpp"
#include "core/Log.hpp"
#include "hook/Hook.hpp"
#include "hook/Input.hpp"
#include "hook/Net.hpp"
#include "render/Ui.hpp"
#include "sig/Sigs.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <map>
#include <mutex>
#include <vector>

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

// The inventory objects hang off the player through an entity registry whose layout changes from session to
// session, so no fixed pointer path reaches them. They are found once per world instead: a heap sweep for
// their vtables ("<name>.vtable", relative to the image), checked by a vector of item stacks at a known spot.
std::atomic<uintptr_t> handObj{0}, armorObj{0}, attrArray{0};
std::atomic<bool> sweeping{false};

uintptr_t imageBase() { return reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)); }

bool stackVector(uintptr_t holder, size_t count) {
    uintptr_t begin = mem::pointer(holder), end = mem::pointer(holder + 8);
    uintptr_t stackVt = imageBase() + uintptr_t(off("stack.vtable"));
    return begin && end - begin == count * size_t(off("stack.size")) && mem::pointer(begin) == stackVt;
}

bool validHand(uintptr_t o) {
    return o && mem::pointer(o + off("hand.tag")) == imageBase() + uintptr_t(off("hand.vtable")) && stackVector(o + off("hand.items"), 36);
}

bool validArmor(uintptr_t o) {
    return o && mem::pointer(o) == imageBase() + uintptr_t(off("armor.vtable")) && stackVector(o + off("armor.items"), size_t(off("armor.count")));
}

// Attribute instances (0x88 bytes: vtable, Attribute definition, ..., max at +0x78, value at +0x7c) sit in one
// array per entity. Only players carry the player.* attributes, and only the local one is sent them by a server.
uintptr_t attrDef(const char* name) { return imageBase() + uintptr_t(sigs::offset(std::string("attr.") + name, 0)); }

uintptr_t attrIn(uintptr_t array, const char* name) {
    uintptr_t vt = imageBase() + uintptr_t(off("attr.vtable")), def = attrDef(name);
    int size = off("attr.size");
    for (int k = 0; k < 32; k++) {
        uintptr_t e = array + uintptr_t(k) * size;
        if (mem::pointer(e) != vt) return 0;
        if (mem::pointer(e + 8) == def) return e;
    }
    return 0;
}

bool validAttrs(uintptr_t array) {
    return array && attrIn(array, "hunger") && attrIn(array, "health");
}

template <class Fn>
void eachPrivateWord(Fn&& fn) {
    std::vector<uint8_t> buf(1 << 20);
    MEMORY_BASIC_INFORMATION mbi{};
    for (uintptr_t at = 0x10000; !client::unloading() && VirtualQuery(reinterpret_cast<void*>(at), &mbi, sizeof(mbi));) {
        uintptr_t start = reinterpret_cast<uintptr_t>(mbi.BaseAddress), next = start + mbi.RegionSize;
        bool ok = mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE && (mbi.Protect & PAGE_READWRITE) && !(mbi.Protect & PAGE_GUARD);
        for (uintptr_t chunk = start; ok && chunk < next && !client::unloading(); chunk += buf.size()) {
            size_t n = std::min<size_t>(buf.size(), next - chunk);
            if (!mem::readBytes(chunk, buf.data(), n)) continue;
            for (size_t i = 0; i + 8 <= n; i += 8) {
                uintptr_t v;
                std::memcpy(&v, buf.data() + i, 8);
                fn(chunk + i, v);
            }
        }
        if (next <= at) break;
        at = next;
    }
}

// Objects of entities from a world left earlier stay in freed memory with the same look, so every candidate is
// kept and the one live structures still point at wins (stale copies have no references left).
uintptr_t mostReferenced(std::vector<uintptr_t>& cands, std::vector<int>& refs) {
    uintptr_t best = 0;
    int bestRefs = -1;
    for (size_t k = 0; k < cands.size(); k++)
        if (refs[k] > bestRefs) bestRefs = refs[k], best = cands[k];
    return best;
}

void sweep() {
    uintptr_t handVt = imageBase() + uintptr_t(off("hand.vtable")), armorVt = imageBase() + uintptr_t(off("armor.vtable"));
    uintptr_t attrVt = imageBase() + uintptr_t(off("attr.vtable")), hungerDef = attrDef("hunger");
    int handTag = off("hand.tag"), attrSize = off("attr.size");
    std::vector<uintptr_t> hands, armors, attrs;
    eachPrivateWord([&](uintptr_t at, uintptr_t v) {
        if (v == handVt && validHand(at - handTag)) hands.push_back(at - handTag);
        else if (v == armorVt && validArmor(at)) armors.push_back(at);
        else if (v == hungerDef && mem::pointer(at - 8) == attrVt) {
            uintptr_t e = at - 8;
            while (mem::pointer(e - attrSize) == attrVt) e -= attrSize;
            if (validAttrs(e) && std::find(attrs.begin(), attrs.end(), e) == attrs.end()) attrs.push_back(e);
        }
    });
    // one more pass counts pointers to (or just into) each candidate
    std::vector<uintptr_t> all;
    for (auto* v : {&hands, &armors, &attrs}) all.insert(all.end(), v->begin(), v->end());
    std::sort(all.begin(), all.end());
    if (client::unloading()) {
        sweeping = false;
        return;
    }
    std::vector<int> count(all.size());
    if (!all.empty())
        eachPrivateWord([&](uintptr_t, uintptr_t v) {
            auto it = std::upper_bound(all.begin(), all.end(), v);
            if (it == all.begin()) return;
            --it;
            if (v - *it < 0x40) count[size_t(it - all.begin())]++;
        });
    auto pick = [&](std::vector<uintptr_t>& cands) {
        std::vector<int> refs;
        for (uintptr_t c : cands) refs.push_back(count[size_t(std::lower_bound(all.begin(), all.end(), c) - all.begin())]);
        return mostReferenced(cands, refs);
    };
    handObj = pick(hands);
    armorObj = pick(armors);
    attrArray = pick(attrs);
    logger::info("live: inventory {} of {}, armor {} of {}, stats {} of {}", handObj ? "found" : "missing", hands.size(), armorObj ? "found" : "missing",
                 armors.size(), attrArray ? "found" : "missing", attrs.size());
    sweeping = false;
}
class Live : public Provider {
public:
    Live() { self = this; }
    ~Live() override {
        detach(false);
        self = nullptr;
    }

    unsigned supports() const override {
        unsigned m = 0;
        if (sigs::address("LocalPlayer") && off("player.posX") >= 0) m |= unsigned(Domain::Player) | unsigned(Domain::Camera);
        if ((sigs::address("Level") || off("level.via0") >= 0) && off("level.time") >= 0) m |= unsigned(Domain::World);
        if ((m & unsigned(Domain::Player)) && off("hit.via0") >= 0) m |= unsigned(Domain::Target) | unsigned(Domain::Combat);
        if ((m & unsigned(Domain::Player)) && sigs::address("AttackEntity")) m |= unsigned(Domain::Combat);
        if ((m & unsigned(Domain::Player)) && off("hand.vtable") >= 0) m |= unsigned(Domain::Inventory);
        if ((m & unsigned(Domain::Player)) && off("chat.via0") >= 0) m |= unsigned(Domain::Chat);
        if ((m & unsigned(Domain::Player)) && off("player.registry") >= 0) m |= unsigned(Domain::Others) | unsigned(Domain::Tab);
        if ((m & unsigned(Domain::Player)) && off("scoreboard.via0") >= 0) m |= unsigned(Domain::Scoreboard);
        if ((m & unsigned(Domain::Player)) && off("pool.effects") != -1) m |= unsigned(Domain::Effects);
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
        if (playerPtr_ && (have & unsigned(Domain::Inventory))) {
            readStats(s);
            readInventory(s, ev);
            readUse(s, ev);
        }
        if (playerPtr_ && (have & unsigned(Domain::Others))) readEntities(s);
        if (playerPtr_ && (have & unsigned(Domain::Effects))) readEffects(s);
        if (playerPtr_ && (have & unsigned(Domain::Scoreboard))) readScoreboard(s);
        if (have & unsigned(Domain::World)) readWorld(s);
        if (playerPtr_ && (have & unsigned(Domain::Chat))) readChat(ev);
        else chatPrimed_ = false;
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
        // without state flags the movement itself tells: walking tops out at 4.3 blocks/s, sprinting at 5.6,
        // and standing on something keeps the vertical speed at zero
        if (off("has.MoveState") > 0 && off("player.sprinting") < 0) {
            float flat = std::hypot(pl.vel.x, pl.vel.z);
            pl.onGround = std::fabs(pl.vel.y) < 0.05f;
            pl.sprinting = flat > 4.9f;
            pl.sneaking = input::grabbed() && (input::down(VK_LSHIFT) || input::down(VK_RSHIFT));
        }
        // the pick ray right behind the eye position points where the player looks, scaled to the pick range
        if (off("player.ray") > 0) {
            float dx = mem::get<float>(pb + px + 12), dy = mem::get<float>(pb + px + 16), dz = mem::get<float>(pb + px + 20);
            float len = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (len > 0.01f && std::isfinite(len)) {
                pl.pitch = std::asin(std::clamp(-dy / len, -1.f, 1.f)) * 57.29578f;
                pl.yaw = std::atan2(-dx, dz) * 57.29578f;
            }
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
        t.breakProgress = 0.f;
        if (t.kind == Target::Kind::Block) {
            t.blockX = mem::get<int>(h + 0x20);
            t.blockY = mem::get<int>(h + 0x24);
            t.blockZ = mem::get<int>(h + 0x28);
            t.breakProgress = breakProgress(t);
        }

        uint32_t rawId = mem::get<uint32_t>(h + 0x48);
        uintptr_t reg = mem::pointer(h + off("hit.registry"));
        if (t.kind == Target::Kind::Entity && reg) describe(t, reg, rawId);
        followHits(reg, ev);

        bool press = input::down(VK_LBUTTON) && input::grabbed() && !ui::wantsCursor();
        bool edge = press && !wasPressed_;
        wasPressed_ = press;
        if (!edge || t.kind != Target::Kind::Entity) return;
        hitId_ = rawId;
        hitAt_ = GetTickCount64();
        hitHealth_ = t.health;
        hitName_ = t.name;
        hitReach_ = t.distance;
        Event e{EventKind::Hit};
        e.reach = t.distance;
        e.crit = !onGround_ && fallSpeed_ < 0.f && !sprinting_;
        e.actor = id;
        e.hasPos = true;
        e.pos = at;
        ev.push_back(std::move(e));
    }

    // Entities live in an entt registry (1.26.52: the pick result's weak reference points at it). Its pool map is a
    // dense_map whose packed nodes sit at "registry.pools" (32 bytes: next, component type hash, shared_ptr to the
    // pool). A pool keeps sparse pages of (version | position) at +0x08, the packed entity list at +0x20 and payload
    // pages at +0x50 (1024 components per page). Ids are 18 bits of entity and 14 of version; a free slot in the
    // packed list carries the version 0x3fff.
    uintptr_t pool(uintptr_t reg, const char* name) {
        uint32_t key = uint32_t(off(name));
        auto& cache = pools_[name];
        if (cache.first == reg && cache.second) return cache.second;
        uintptr_t begin = mem::pointer(reg + off("registry.pools")), end = mem::pointer(reg + off("registry.pools") + 8);
        cache = {reg, 0};
        for (uintptr_t n = begin; n && n < end && n < begin + 0x10000 * 32; n += 32)
            if (mem::get<uint32_t>(n + 8) == key) {
                cache.second = mem::pointer(n + 0x10);
                break;
            }
        return cache.second;
    }

    static uintptr_t component(uintptr_t p, uint32_t id, int size) {
        if (!p) return 0;
        constexpr uint32_t mask = 0x3ffff;
        uint32_t idx = id & mask;
        uintptr_t sparse = mem::pointer(p + 8), sparseEnd = mem::pointer(p + 0x10);
        if (!sparse || (idx / 4096) * 8 >= sparseEnd - sparse) return 0;
        uintptr_t page = mem::pointer(sparse + (idx / 4096) * 8);
        uint32_t entry = page ? mem::get<uint32_t>(page + (idx % 4096) * 4, 0xffffffff) : 0xffffffff;
        if (entry == 0xffffffff) return 0;
        uint32_t pos = entry & mask;
        uintptr_t packed = mem::pointer(p + 0x20);
        if (mem::get<uint32_t>(packed + uintptr_t(pos) * 4) != id) return 0;
        uintptr_t payload = mem::pointer(mem::pointer(p + 0x50) + (pos / 1024) * 8);
        return payload ? payload + uintptr_t(pos % 1024) * size : 0;
    }

    void describe(Target& t, uintptr_t reg, uint32_t id) {
        t.name.clear();
        t.isPlayer = false;
        if (uintptr_t def = component(pool(reg, "pool.identifier"), id, off("identifier.size"))) {
            t.name = text(def + off("identifier.name"));
            t.isPlayer = t.name == "player";
        }
        if (t.isPlayer) {
            std::string shown = playerName(reg, id);
            if (!shown.empty()) t.name = shown;
        }
        // primed TNT burns for 80 ticks from the moment it appears
        auto seen = firstSeen_.find(id);
        t.fuse = t.name == "tnt" && seen != firstSeen_.end() ? std::max(0.f, 4.f - float(GetTickCount64() - seen->second) / 1000.f) : 0.f;
        uintptr_t attrs = component(pool(reg, "pool.attributes"), id, off("attributes.size"));
        uintptr_t first = attrs ? mem::pointer(attrs + off("attributes.list")) : 0;
        if (uintptr_t hp = first ? attrIn(first, "health") : 0) {
            t.health = mem::get<float>(hp + off("attr.value"));
            t.maxHealth = mem::get<float>(hp + off("attr.max"));
        } else {
            t.health = 0.f;
            t.maxHealth = 0.f;
        }
    }

    // SynchedActorData keeps a vector of data items (vtable, type byte at +8, id at +0xa, value at +0x10); item 4
    // is the name tag, a string (type 4)
    std::string playerName(uintptr_t reg, uint32_t id) {
        uintptr_t data = component(pool(reg, "pool.synched"), id, off("synched.size"));
        if (!data) return {};
        uintptr_t begin = mem::pointer(data), end = mem::pointer(data + 8);
        int slot = off("synched.name");
        if (!begin || end <= begin + uintptr_t(slot) * 8) return {};
        uintptr_t item = mem::pointer(begin + uintptr_t(slot) * 8);
        if (!item || mem::get<uint8_t>(item + 8) != 4) return {};
        return text(item + 0x10);
    }

    // The game mode (1.26.52: actor+0xaa0) keeps the block being broken at +0x10 and its progress (0..1) at +0x24.
    // The actor object is the one the registry's ActorOwnerComponent holds, not the LocalPlayer global.
    float breakProgress(const Target& t) {
        uintptr_t reg = mem::pointer(playerPtr_ + off("player.registry"));
        uintptr_t owner = component(pool(reg, "pool.owner"), selfId(reg), 8);
        uintptr_t gm = owner ? mem::pointer(mem::pointer(owner) + off("actor.gameMode")) : 0;
        if (!gm) return 0.f;
        int at = off("gameMode.block");
        if (mem::get<int>(gm + at) != t.blockX || mem::get<int>(gm + at + 4) != t.blockY || mem::get<int>(gm + at + 8) != t.blockZ) return 0.f;
        float p = mem::get<float>(gm + off("gameMode.progress"));
        return std::isfinite(p) ? std::clamp(p, 0.f, 1.f) : 0.f;
    }

    // only the local player has a LocalPlayerComponent, an empty tag, so that pool's single entity is us
    uint32_t selfId(uintptr_t reg) {
        uintptr_t p = pool(reg, "pool.localPlayer");
        uintptr_t packed = p ? mem::pointer(p + 0x20) : 0;
        return packed && mem::pointer(p + 0x28) > packed ? mem::get<uint32_t>(packed, 0xffffffff) : 0xffffffff;
    }

    static float health(uintptr_t attrs, float* max) {
        uintptr_t first = attrs ? mem::pointer(attrs + off("attributes.list")) : 0;
        uintptr_t hp = first ? attrIn(first, "health") : 0;
        if (!hp) return 0.f;
        if (max) *max = mem::get<float>(hp + off("attr.max"));
        return mem::get<float>(hp + off("attr.value"));
    }

    // Every actor carries an identifier component, so its pool's packed list is the list of loaded entities.
    // Positions come from the render position (interpolated between ticks); the state vector holds the tick
    // position, the one before and the motion, and the box bottom tells how far the position sits above the feet.
    void readEntities(State& s) {
        uintptr_t reg = mem::pointer(playerPtr_ + off("player.registry"));
        uint32_t self = selfId(reg);
        if (std::string me = playerName(reg, self); !me.empty()) s.player.name = me;
        s.others.clear();
        s.shots.clear();
        s.tab.clear();
        if (!s.player.name.empty()) s.tab.push_back(TabEntry{s.player.name});
        s.world.entities = 0;
        s.world.players = 0;
        uintptr_t ids = pool(reg, "pool.identifier");
        if (!ids) return;
        uintptr_t packed = mem::pointer(ids + 0x20), packedEnd = mem::pointer(ids + 0x28);
        if (!packed || packedEnd <= packed || packedEnd - packed > 4 * 20000) return;
        std::vector<uint32_t> list((packedEnd - packed) / 4);
        if (!mem::readBytes(packed, list.data(), list.size() * 4)) return;
        uintptr_t states = pool(reg, "pool.state"), boxes = pool(reg, "pool.aabb"), render = pool(reg, "pool.render");
        uintptr_t attrPool = pool(reg, "pool.attributes");
        uint64_t now = GetTickCount64();
        std::map<uint32_t, uint64_t> seen;
        for (uint32_t id : list) {
            if ((id >> 18) == 0x3fff) continue;
            uintptr_t def = component(ids, id, off("identifier.size"));
            uintptr_t st = component(states, id, off("state.size"));
            if (!def || !st) continue;
            auto first = firstSeen_.find(id);
            bool fresh = first == firstSeen_.end();
            seen[id] = fresh ? now : first->second;
            if (id == self) continue;
            std::string kind = text(def + off("identifier.name"));
            Vec3 tick{mem::get<float>(st), mem::get<float>(st + 4), mem::get<float>(st + 8)};
            Vec3 motion{mem::get<float>(st + 24), mem::get<float>(st + 28), mem::get<float>(st + 32)};
            Vec3 pos = tick;
            if (uintptr_t r = component(render, id, off("render.size"))) pos = {mem::get<float>(r), mem::get<float>(r + 4), mem::get<float>(r + 8)};
            if (uintptr_t box = component(boxes, id, off("aabb.size"))) pos.y -= tick.y - mem::get<float>(box + 4);
            s.world.entities++;
            bool arrow = kind == "arrow", pearl = kind == "ender_pearl", trident = kind == "thrown_trident";
            if (arrow || pearl || trident) {
                Projectile pr;
                pr.id = id;
                pr.kind = arrow ? 0 : pearl ? 1 : 2;
                pr.pos = pos;
                pr.vel = {motion.x * 20.f, motion.y * 20.f, motion.z * 20.f};
                // the shooter is not linked here; a projectile that shows up right at the eyes was ours
                bool& mine = mine_[id];
                if (fresh) mine = distance(pos, eye_) < 2.5f;
                pr.mine = mine;
                s.shots.push_back(pr);
                continue;
            }
            float max = 0.f, hp = health(component(attrPool, id, off("attributes.size")), &max);
            if (max <= 0.f) continue;
            Other o;
            o.id = id;
            o.kind = kind;
            o.isPlayer = kind == "player";
            o.name = o.isPlayer ? playerName(reg, id) : kind;
            o.pos = pos;
            o.health = hp;
            o.maxHealth = max;
            if (o.isPlayer) {
                s.world.players++;
                if (!o.name.empty()) s.tab.push_back(TabEntry{o.name});
            }
            s.others.push_back(std::move(o));
        }
        firstSeen_ = std::move(seen);
        std::erase_if(mine_, [&](auto& kv) { return !firstSeen_.count(kv.first); });
        s.world.players++;
    }

    // MobEffectsComponent is a vector of effect instances indexed by effect id (0x90 bytes each: id, duration in
    // ticks at +4, amplifier at +0x20); unused slots stay zero
    void readEffects(State& s) {
        static const char* names[] = {"", "speed", "slowness", "haste", "mining_fatigue", "strength", "instant_health", "instant_damage",
                                      "jump_boost", "nausea", "regeneration", "resistance", "fire_resistance", "water_breathing",
                                      "invisibility", "blindness", "night_vision", "hunger", "weakness", "poison", "wither",
                                      "health_boost", "absorption", "saturation", "levitation", "fatal_poison", "conduit_power",
                                      "slow_falling", "bad_omen", "village_hero", "darkness", "trial_omen", "wind_charged",
                                      "weaving", "oozing", "infested", "raid_omen"};
        static const uint32_t colors[] = {0, 0x7CAFC6, 0x5A6C81, 0xD9C043, 0x4A4217, 0x932423, 0xF82423, 0x430A09, 0x22FF4C, 0x551D4A,
                                          0xCD5CAB, 0x99453A, 0xE49A3A, 0x2E5299, 0x7F8392, 0x1F1F23, 0x1F1FA1, 0x587653, 0x484D48,
                                          0x4E9331, 0x352A27, 0xF87D23, 0x2552A5, 0xF82423, 0xCEFFFF, 0x4E9331, 0x1DC2D1, 0xFFEFD1,
                                          0x0B6138, 0x44FF44, 0x292721, 0x1BBDB5, 0xBDC9FF, 0x78695A, 0x99FFA3, 0x8C9B8C, 0xDE4058};
        static const bool bad[] = {false, false, true, false, true, false, false, true, false, true, false, false, false, false,
                                   false, true, false, true, true, true, true, false, false, false, true, true, false, false,
                                   true, false, true, true, true, true, true, true, true};
        uintptr_t reg = mem::pointer(playerPtr_ + off("player.registry"));
        uint32_t self = selfId(reg);
        auto& list = s.player.effects;
        list.clear();
        uintptr_t fx = component(pool(reg, "pool.effects"), self, off("effects.size"));
        if (!fx) return;
        uintptr_t begin = mem::pointer(fx), end = mem::pointer(fx + 8);
        int size = off("effect.size");
        if (!begin || end <= begin || (end - begin) / size > 64) return;
        for (uintptr_t e = begin; e + size <= end; e += size) {
            uint32_t id = mem::get<uint32_t>(e);
            int ticks = mem::get<int>(e + off("effect.duration"));
            if (id == 0 || id >= std::size(names) || ticks <= 0) continue;
            Effect fxe;
            fxe.id = names[id];
            fxe.amplifier = mem::get<int>(e + off("effect.amplifier"));
            fxe.seconds = float(ticks) / 20.f;
            // the total is not kept, so the longest time seen since the effect started stands in for it
            float& longest = effectTotal_[id];
            if (fxe.seconds > longest) longest = fxe.seconds;
            fxe.total = longest;
            fxe.good = !bad[id];
            fxe.color = colors[id];
            list.push_back(std::move(fxe));
        }
        std::erase_if(effectTotal_, [&](auto& kv) { return std::none_of(list.begin(), list.end(), [&](auto& e) { return e.id == names[kv.first]; }); });
    }


    // The client scoreboard (1.26.52: player+0x490) keeps its display slots in an unordered_map at +0x18 (list head,
    // nodes: next, prev, slot name at +0x10, objective at +0x30, sort order at +0x38). An objective has its scores
    // in an unordered_map whose list head is at +0x20 (nodes: scoreboard id at +0x10, identity at +0x18, score at
    // +0x20), its name at +0x58 and its display name at +0x78. A fake player identity holds its name at +0x28.
    template <class Fn>
    static void eachNode(uintptr_t head, Fn&& fn) {
        if (!head) return;
        uintptr_t n = mem::pointer(head);
        for (int k = 0; n && n != head && k < 256; k++, n = mem::pointer(n)) fn(n);
    }

    void readScoreboard(State& s) {
        auto& board = s.scoreboard;
        board.title.clear();
        board.lines.clear();
        uintptr_t sb = follow(playerPtr_, "scoreboard");
        if (!sb) return;
        uintptr_t objective = 0;
        bool ascending = false;
        eachNode(mem::pointer(sb + off("scoreboard.slots")), [&](uintptr_t n) {
            if (objective || text(n + 0x10) != "sidebar") return;
            objective = mem::pointer(n + 0x30);
            ascending = mem::get<uint8_t>(n + 0x38) == 0;
        });
        if (!objective) return;
        board.title = text(objective + off("objective.title"));
        eachNode(mem::pointer(objective + off("objective.scores")), [&](uintptr_t n) {
            uintptr_t id = mem::pointer(n + 0x18);
            std::string name = id ? text(id + off("identity.name")) : std::string();
            if (!name.empty()) board.lines.push_back({std::move(name), mem::get<int>(n + 0x20)});
        });
        std::stable_sort(board.lines.begin(), board.lines.end(), [&](auto& a, auto& b) { return ascending ? a.second < b.second : a.second > b.second; });
    }

    // Cameras are entities too. A system turns the player to match every camera that has an
    // UpdatePlayerFromCameraComponent; it finds them through an entt view, which checks membership by comparing the
    // version bits of the pool's sparse entry with the entity. Flipping one version bit makes the view skip the
    // camera while plain lookups (which only use the position bits) still work, so the camera keeps turning with
    // the mouse and the player stays put. Before the link comes back, the cameras get their old angles back
    // (CameraDirectLookComponent: yaw, pitch in radians), otherwise the player would snap to where the camera looked.
public:
    bool detach(bool on) {
        uintptr_t reg = playerPtr_ ? mem::pointer(playerPtr_ + off("player.registry")) : 0;
        if (detached_.reg && detached_.reg != reg) detached_ = {};
        if (!on) {
            if (!detached_.reg) return false;
            for (auto& [at, angles] : detached_.angles) mem::write(at, angles);
            for (auto& [at, entry] : detached_.entries) mem::write(at, entry);
            detached_ = {};
            return false;
        }
        uintptr_t link = pool(reg, "pool.cameraLink");
        if (!reg || !link) return false;
        uintptr_t look = pool(reg, "pool.cameraLook");
        if (!detached_.reg) {
            detached_.reg = reg;
            eachEntity(look, [&](uint32_t id) {
                if (uintptr_t c = component(look, id, off("cameraLook.size"))) detached_.angles.push_back({c, mem::get<uint64_t>(c)});
            });
        }
        eachEntity(link, [&](uint32_t id) {
            uintptr_t at = sparseEntry(link, id);
            if (!at) return;
            uint32_t entry = mem::get<uint32_t>(at);
            auto known = std::find_if(detached_.entries.begin(), detached_.entries.end(), [&](auto& e) { return e.first == at; });
            if (known == detached_.entries.end()) detached_.entries.push_back({at, entry});
            else if (entry == (known->second ^ (1u << 18))) return;
            else known->second = entry;
            mem::write(at, entry ^ (1u << 18));
        });
        return true;
    }

private:
    struct Detached {
        uintptr_t reg = 0;
        std::vector<std::pair<uintptr_t, uint32_t>> entries;
        std::vector<std::pair<uintptr_t, uint64_t>> angles;
    };
    Detached detached_;

    static uintptr_t sparseEntry(uintptr_t p, uint32_t id) {
        uint32_t idx = id & 0x3ffff;
        uintptr_t sparse = mem::pointer(p + 8), sparseEnd = mem::pointer(p + 0x10);
        if (!sparse || (idx / 4096) * 8 >= sparseEnd - sparse) return 0;
        uintptr_t page = mem::pointer(sparse + (idx / 4096) * 8);
        return page ? page + (idx % 4096) * 4 : 0;
    }

    template <class Fn>
    static void eachEntity(uintptr_t p, Fn&& fn) {
        if (!p) return;
        uintptr_t packed = mem::pointer(p + 0x20), end = mem::pointer(p + 0x28);
        for (uintptr_t a = packed; a && a < end && a < packed + 4 * 256; a += 4)
            if (uint32_t id = mem::get<uint32_t>(a, 0xffffffff); (id >> 18) != 0x3fff) fn(id);
    }

    // a hit counts as landed when the struck entity loses health shortly after; down to zero is a kill
    void followHits(uintptr_t reg, std::vector<Event>& ev) {
        if (!hitId_ || !reg) return;
        if (GetTickCount64() - hitAt_ > 1500) {
            hitId_ = 0;
            return;
        }
        uintptr_t attrs = component(pool(reg, "pool.attributes"), hitId_, off("attributes.size"));
        uintptr_t first = attrs ? mem::pointer(attrs + off("attributes.list")) : 0;
        uintptr_t hp = first ? attrIn(first, "health") : 0;
        float now = hp ? mem::get<float>(hp + off("attr.value")) : 0.f;
        if (!hp || now >= hitHealth_ - 0.01f) return;
        Event c{EventKind::Confirm};
        c.value = float(GetTickCount64() - hitAt_);
        c.damage = hitHealth_ - now;
        c.reach = hitReach_;
        c.actor = hitId_;
        c.text = hitName_;
        ev.push_back(c);
        if (now <= 0.f) {
            Event k{EventKind::Kill};
            k.actor = hitId_;
            k.text = hitName_;
            ev.push_back(std::move(k));
        }
        hitId_ = 0;
    }

    // ItemStack (1.26.52, 0x98 bytes): +0x8 weak pointer to the Item, +0x10 CompoundTag user data, +0x20 aux,
    // +0x22 count. Item: +0x128 full name ("minecraft:arrow"), +0x150 max damage.
    static std::string text(uintptr_t at) {
        size_t len = mem::get<size_t>(at + 16), cap = mem::get<size_t>(at + 24);
        if (len == 0 || len > 128) return {};
        uintptr_t p = cap > 15 ? mem::pointer(at) : at;
        std::string out(len, '\0');
        return p && mem::readBytes(p, out.data(), len) ? out : std::string{};
    }

    // user data is a CompoundTag: a std::map<std::string, tag> (node: left, parent, right, color, isnil, key at
    // +0x20, tag at +0x40 with its payload 8 bytes in)
    static uintptr_t findTag(uintptr_t tag, const char* key) {
        uintptr_t head = mem::pointer(tag + 8);
        if (!head) return 0;
        uintptr_t stack[32];
        int n = 0, seen = 0;
        if (uintptr_t root = mem::pointer(head + 8); root && root != head) stack[n++] = root;
        while (n > 0 && seen++ < 64) {
            uintptr_t node = stack[--n];
            if (mem::get<uint8_t>(node + 0x19, 1)) continue;
            if (text(node + 0x20) == key) return node + 0x40;
            for (int side : {0, 16}) {
                uintptr_t c = mem::pointer(node + side);
                if (c && c != head && n < 32) stack[n++] = c;
            }
        }
        return 0;
    }

    static Item item(uintptr_t stack) {
        Item it;
        int count = mem::get<uint8_t>(stack + off("stack.count"));
        uintptr_t def = mem::pointer(mem::pointer(stack + off("stack.item")));
        if (!count || !def) return it;
        it.name = text(def + off("item.name"));
        if (it.name.starts_with("minecraft:")) it.name.erase(0, 10);
        it.count = count;
        it.aux = mem::get<int16_t>(stack + off("stack.aux"));
        it.maxDamage = mem::get<int16_t>(def + off("item.maxDamage"));
        if (uintptr_t tag = mem::pointer(stack + off("stack.tag"))) {
            if (uintptr_t d = findTag(tag, "Damage")) it.damage = mem::get<int>(d + 8);
            it.enchanted = findTag(tag, "ench") != 0;
        }
        return it;
    }

    // The HUD keeps the received chat lines in a std::vector of GuiMessage (1.26.52: 0x110 bytes, finished line
    // with sender as a std::string at +0x90). New lines are the ones after the last line seen; the vector is
    // trimmed from the front, so the last seen line is searched from the end.
    void readChat(std::vector<Event>& ev) {
        uintptr_t vec = follow(playerPtr_, "chat") + off("chat.lines");
        uintptr_t begin = mem::pointer(vec), end = mem::pointer(vec + 8);
        int size = off("chat.size");
        if (!begin || end < begin || (end - begin) % size || (end - begin) / size > 1000) return;
        size_t count = (end - begin) / size;
        auto line = [&](size_t k) { return text(begin + k * size + off("chat.text")); };
        if (!chatPrimed_) {
            chatPrimed_ = true;
            chatLast_ = count ? line(count - 1) : std::string();
            chatCount_ = count;
            return;
        }
        if (count == 0) {
            chatLast_.clear();
            chatCount_ = 0;
            return;
        }
        if (count == chatCount_ && line(count - 1) == chatLast_) return;
        size_t from = 0;
        if (!chatLast_.empty())
            for (size_t k = count; k-- > 0;)
                if (line(k) == chatLast_) {
                    from = k + 1;
                    break;
                }
        for (size_t k = from; k < count; k++) {
            Event e{EventKind::Chat};
            e.text = line(k);
            if (!e.text.empty()) ev.push_back(std::move(e));
        }
        chatLast_ = line(count - 1);
        chatCount_ = count;
    }

    void readStats(State& s) {
        uintptr_t array = attrArray;
        if (off("player.registry") >= 0) {
            uintptr_t reg = mem::pointer(playerPtr_ + off("player.registry"));
            uintptr_t attrs = component(pool(reg, "pool.attributes"), selfId(reg), off("attributes.size"));
            if (uintptr_t first = attrs ? mem::pointer(attrs + off("attributes.list")) : 0; validAttrs(first)) array = first;
        }
        auto& pl = s.player;
        pl.statsKnown = validAttrs(array);
        if (!pl.statsKnown) return;
        auto value = [&](const char* name, float fallback) {
            uintptr_t e = attrIn(array, name);
            return e ? mem::get<float>(e + off("attr.value"), fallback) : fallback;
        };
        if (uintptr_t h = attrIn(array, "health")) {
            pl.health = mem::get<float>(h + off("attr.value"), pl.health);
            pl.maxHealth = mem::get<float>(h + off("attr.max"), pl.maxHealth);
        }
        pl.absorption = value("absorption", pl.absorption);
        pl.hunger = value("hunger", pl.hunger);
        pl.saturation = value("saturation", pl.saturation);
        pl.level = int(value("level", float(pl.level)));
        pl.xp = value("experience", pl.xp);
    }

    // Using an item is the use button held while the hand holds something usable: a bow draws for one second to
    // full power, a thrown pearl or potion shows as the held stack shrinking right after a press.
    void readUse(State& s, std::vector<Event>& ev) {
        auto& pl = s.player;
        const Item& held = pl.held();
        bool press = input::down(VK_RBUTTON) && input::grabbed() && !ui::wantsCursor();
        double now = ui::time();
        bool drawable = held.name == "bow" || held.name == "crossbow" || held.name == "trident";
        bool usable = drawable || held.name == "shield" || held.name.find("potion") != std::string::npos ||
                      held.name.starts_with("cooked_") || held.name.find("apple") != std::string::npos || held.name == "bread";
        if (press && !usePressed_) {
            useStart_ = now;
            pressItem_ = held.name;
            pressCount_ = held.count;
            pressSlot_ = pl.slot;
        }
        if (!press && usePressed_ && pressItem_ == "bow" && now - useStart_ > 0.1) {
            Event e{EventKind::BowRelease};
            e.value = float(std::min(1.0, (now - useStart_) / 1.0));
            e.item = "bow";
            ev.push_back(std::move(e));
        }
        usePressed_ = press;
        pl.usingItem = press && usable && held.name == pressItem_;
        pl.useProgress = pl.usingItem ? float(std::min(1.0, (now - useStart_) / (drawable ? 1.0 : 1.6))) : 0.f;
        // a press that made the held stack shrink used one up
        if (!pressItem_.empty() && now - useStart_ < 0.6 && pl.slot == pressSlot_ && held.name == pressItem_ && held.count < pressCount_) {
            Event e{EventKind::ItemUse};
            e.item = pressItem_;
            ev.push_back(std::move(e));
            pressCount_ = held.count;
        }
    }

    // a totem used up shows as one fewer totem in the hands while health is low
    void countTotems(const Player& pl, std::vector<Event>& ev) {
        int n = 0;
        for (const Item* it : {&pl.offhand, &pl.held()})
            if (it->name == "totem_of_undying") n += it->count;
        if (totems_ >= 0 && n < totems_ && pl.health <= 8.f) ev.push_back(Event{EventKind::TotemPop});
        totems_ = n;
    }

    void readInventory(State& s, std::vector<Event>& ev) {
        // ten times a second is plenty for counters and armor, and keeps the reads off every frame
        uint64_t now = GetTickCount64();
        if (now - inventoryAt_ < 100) return;
        inventoryAt_ = now;
        auto& pl = s.player;
        int size = off("stack.size");
        uintptr_t hand = handObj, armor = armorObj;
        bool lost = !validHand(hand) || !validArmor(armor) || !validAttrs(attrArray);
        if (lost && !sweeping && now - sweptAt_ > 3000) {
            sweptAt_ = now;
            sweeping = true;
            bg::run(sweep);
        }
        if (validHand(hand)) {
            uintptr_t items = mem::pointer(hand + off("hand.items"));
            for (int k = 0; k < 9; k++) pl.hotbar[size_t(k)] = item(items + uintptr_t(k) * size);
            pl.main.resize(27);
            for (int k = 0; k < 27; k++) pl.main[size_t(k)] = item(items + uintptr_t(k + 9) * size);
            pl.offhand = item(hand + off("hand.offhand"));
            countTotems(pl, ev);
            // right after the offhand sits a copy of the selected stack; the slot is the hotbar stack it copies
            uintptr_t held = hand + off("hand.held");
            uintptr_t heldItem = mem::pointer(held + off("stack.item"));
            int heldCount = mem::get<uint8_t>(held + off("stack.count"));
            for (int k = 0; k < 9; k++) {
                uintptr_t st = items + uintptr_t(k) * size;
                if (mem::pointer(st + off("stack.item")) == heldItem && mem::get<uint8_t>(st + off("stack.count")) == heldCount) {
                    pl.slot = k;
                    break;
                }
            }
        }
        if (validArmor(armor)) {
            uintptr_t items = mem::pointer(armor + off("armor.items"));
            for (int k = 0; k < 4; k++) pl.armor[size_t(k)] = item(items + uintptr_t(k) * size);
        }
    }

    void readWorld(State& s) {
        // 1.26.52: the player keeps a pointer to its level, which holds the time of day as an int
        uintptr_t lv = off("level.via0") >= 0 ? (playerPtr_ ? follow(playerPtr_, "level") : 0) : mem::pointer(sigs::address("Level"));
        if (!lv) return;
        s.world.time = int(((mem::get<int>(lv + off("level.time"), s.world.time) % 24000) + 24000) % 24000);
        s.world.day = mem::get<int>(lv + off("level.time")) / 24000 + 1;
        s.world.raining = f(lv, "level.rain", 0.f) > 0.05f;
        s.world.thundering = f(lv, "level.thunder", 0.f) > 0.05f;
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
    uint64_t inventoryAt_ = 0;
    std::map<std::string, std::pair<uintptr_t, uintptr_t>> pools_;
    uint32_t hitId_ = 0;
    uint64_t hitAt_ = 0;
    float hitHealth_ = 0.f;
    float hitReach_ = 0.f;
    std::map<uint32_t, uint64_t> firstSeen_;
    std::map<uint32_t, bool> mine_;
    std::map<uint32_t, float> effectTotal_;
    std::string hitName_;
    uint64_t sweptAt_ = 0;
    int totems_ = -1;
    bool usePressed_ = false;
    double useStart_ = 0.0;
    std::string pressItem_;
    int pressCount_ = 0;
    int pressSlot_ = 0;
    std::string chatLast_;
    size_t chatCount_ = 0;
    bool chatPrimed_ = false;
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

bool freeCamera(bool on) { return self && self->detach(on); }

}
