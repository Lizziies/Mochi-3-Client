#include "Providers.hpp"
#include "Memory.hpp"
#include "core/Bg.hpp"
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

void sweep() {
    uintptr_t handVt = imageBase() + uintptr_t(off("hand.vtable")), armorVt = imageBase() + uintptr_t(off("armor.vtable"));
    int handTag = off("hand.tag");
    uintptr_t hand = 0, armor = 0;
    std::vector<uint8_t> buf(1 << 20);
    MEMORY_BASIC_INFORMATION mbi{};
    // other players (the local server's copy, previews) carry the same objects; the one with items wins
    auto filled = [](uintptr_t items, size_t count) {
        int n = 0;
        for (size_t k = 0; k < count; k++) n += mem::get<uint8_t>(items + k * size_t(off("stack.size")) + off("stack.count")) != 0;
        return n;
    };
    int handBest = -1, armorBest = -1;
    uintptr_t attrVt = imageBase() + uintptr_t(off("attr.vtable")), hungerDef = attrDef("hunger"), attrs = 0;
    for (uintptr_t at = 0x10000; VirtualQuery(reinterpret_cast<void*>(at), &mbi, sizeof(mbi));) {
        uintptr_t start = reinterpret_cast<uintptr_t>(mbi.BaseAddress), next = start + mbi.RegionSize;
        bool ok = mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE && (mbi.Protect & PAGE_READWRITE) && !(mbi.Protect & PAGE_GUARD);
        for (uintptr_t chunk = start; ok && chunk < next; chunk += buf.size()) {
            size_t n = std::min<size_t>(buf.size(), next - chunk);
            if (!mem::readBytes(chunk, buf.data(), n)) continue;
            for (size_t i = 0; i + 8 <= n; i += 8) {
                uintptr_t v;
                std::memcpy(&v, buf.data() + i, 8);
                if (v == handVt && validHand(chunk + i - handTag)) {
                    uintptr_t o = chunk + i - handTag;
                    int f = filled(mem::pointer(o + off("hand.items")), 36) + (mem::get<uint8_t>(o + off("hand.offhand") + off("stack.count")) != 0);
                    if (f > handBest) handBest = f, hand = o;
                } else if (!attrs && v == hungerDef && i >= 8 && mem::pointer(chunk + i - 8) == attrVt) {
                    // walk back to the first instance of the array
                    uintptr_t e = chunk + i - 8;
                    while (mem::pointer(e - off("attr.size")) == attrVt) e -= off("attr.size");
                    if (validAttrs(e)) attrs = e;
                } else if (v == armorVt && validArmor(chunk + i)) {
                    int f = filled(mem::pointer(chunk + i + off("armor.items")), size_t(off("armor.count")));
                    if (f > armorBest) armorBest = f, armor = chunk + i;
                }
            }
        }
        if (next <= at) break;
        at = next;
    }
    handObj = hand;
    armorObj = armor;
    attrArray = attrs;
    logger::info("live: inventory {}, armor {}, stats {}", hand ? "found" : "missing", armor ? "found" : "missing", attrs ? "found" : "missing");
    sweeping = false;
}

class Live : public Provider {
public:
    Live() { self = this; }
    ~Live() override { self = nullptr; }

    unsigned supports() const override {
        unsigned m = 0;
        if (sigs::address("LocalPlayer") && off("player.posX") >= 0) m |= unsigned(Domain::Player) | unsigned(Domain::Camera);
        if ((sigs::address("Level") || off("level.via0") >= 0) && off("level.time") >= 0) m |= unsigned(Domain::World);
        if ((m & unsigned(Domain::Player)) && off("hit.via0") >= 0) m |= unsigned(Domain::Target) | unsigned(Domain::Combat);
        if ((m & unsigned(Domain::Player)) && sigs::address("AttackEntity")) m |= unsigned(Domain::Combat);
        if ((m & unsigned(Domain::Player)) && off("hand.vtable") >= 0) m |= unsigned(Domain::Inventory);
        if ((m & unsigned(Domain::Player)) && off("chat.via0") >= 0) m |= unsigned(Domain::Chat);
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
            readInventory(s);
        }
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
        if (!validAttrs(array)) return;
        auto& pl = s.player;
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

    void readInventory(State& s) {
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
    uint64_t sweptAt_ = 0;
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

}
