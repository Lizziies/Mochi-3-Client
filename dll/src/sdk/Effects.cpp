#include "Effects.hpp"
#include "Game.hpp"
#include "Memory.hpp"
#include "render/Ui.hpp"
#include "core/Log.hpp"
#include "hook/Hook.hpp"
#include "sig/Sigs.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <utility>

namespace fx {

namespace {

constexpr size_t count = size_t(Id::Count);

constexpr Info table[count] = {
    {"fx.fov", "Field of view", Kind::Value},
    {"fx.fovEffects", "FOV effects", Kind::Value},
    {"fx.gamma", "Brightness", Kind::Value},
    {"fx.viewBob", "Camera bobbing", Kind::Skip},
    {"fx.handBob", "Hand bobbing", Kind::Skip},
    {"fx.hurtCam", "Hurt camera shake", Kind::Value},
    {"fx.bobStrength", "Bobbing strength", Kind::Value},
    {"fx.perspective", "Perspective", Kind::Int},
    {"fx.sneakCam", "Sneak camera", Kind::Value},
    {"fx.sensitivity", "Sensitivity", Kind::Value},
    {"fx.time", "Time of day", Kind::Value},
    {"fx.rain", "Rain", Kind::Value},
    {"fx.thunder", "Thunderstorm", Kind::Value},
    {"fx.clouds", "Clouds", Kind::Flag},
    {"fx.sky", "Sky", Kind::Flag},
    {"fx.particles", "Particles", Kind::Flag},
    {"fx.blockEntities", "Block-Entities", Kind::Flag},
    {"fx.shadows", "Shadow", Kind::Flag},
    {"fx.fog", "Fog", Kind::Flag},
    {"fx.vignette", "Vignette", Kind::Flag},
    {"fx.fire", "Fire overlay", Kind::Value},
    {"fx.hitbox", "Hitboxes", Kind::Flag},
    {"fx.hitboxColor", "Hitbox color", Kind::Data},
    {"fx.glintColor", "Glint color", Kind::Data},
    {"fx.hurtColor", "Hurt color", Kind::Data},
    {"fx.fogColor", "Fog color", Kind::Out},
    {"fx.waterColor", "Water color", Kind::Out},
    {"fx.particleScale", "Particle amount", Kind::Value},
    {"fx.guiScale", "GUI scale", Kind::Value},
    {"fx.hideHand", "Hide hand", Kind::Skip},
    {"fx.hideOffhand", "Hide offhand", Kind::Skip},
    {"fx.hideChat", "Hide the original chat", Kind::Skip},
    {"fx.hideScoreboard", "Hide the original scoreboard", Kind::Skip},
    {"fx.hideCrosshair", "Hide the original crosshair", Kind::Skip},
    {"fx.hideHud", "Hide the original HUD", Kind::Skip},
    {"fx.handMatrix", "Hand transform", Kind::Out},
    {"fx.lookTurn", "Player look rotation", Kind::Skip},
    {"fx.lookCamera", "Camera rotation", Kind::Out},
    {"fx.lookDelta", "Look movement", Kind::Out},
    {"fx.selfNametag", "Own nametag", Kind::Flag},
    {"fx.itemPhysics", "Item physics", Kind::Flag},
    {"fx.useDelay", "Item use delay", Kind::Value},
    {"fx.inventoryDelay", "Inventory delay", Kind::Value},
    {"fx.hurtAnim", "Hurt animation", Kind::Flag},
    {"fx.blockOutline", "Original block outline", Kind::Skip},
    {"fx.swingSpeed", "Swing duration", Kind::Value},
    {"fx.crystalHide", "Hide crystal instantly", Kind::Flag},
    {"fx.crystalSimple", "Crystal without spin and bobbing", Kind::Flag},
    {"fx.crystalNoBase", "Crystal without base", Kind::Flag},
    {"fx.ghostRender", "Hide locally removed entities", Kind::Ghost},
    {"fx.ghostPick", "Ignore locally removed entities when aiming", Kind::Filter},
    {"fx.critParticle", "Critical hit particles", Kind::Flag},
    {"fx.hitboxEye", "Hitbox eye line", Kind::Flag},
    {"fx.hitboxEyeColor", "Hitbox eye line color", Kind::Data},
    {"fx.hitboxLook", "Hitbox look line", Kind::Flag},
    {"fx.hitboxLookColor", "Hitbox look line color", Kind::Data},
    {"fx.hitboxLookLength", "Hitbox look line length", Kind::Data},
    {"fx.hitboxWidth", "Hitbox line width", Kind::Data},
    {"fx.hitboxSelf", "Own hitbox", Kind::Flag},
    {"fx.hitboxJava", "Java hitbox size", Kind::Flag},
    {"fx.hitboxRange", "Hitbox range", Kind::Data},
    {"fx.hitbox2D", "Flat hitbox", Kind::Flag},
    {"fx.itemFov", "Item field of view", Kind::Value},
    {"fx.handMatrixThird", "Hand transform in third person", Kind::Out},
    {"fx.renderEntities", "Draw entities", Kind::Flag},
    {"fx.renderTerrain", "Draw terrain", Kind::Flag},
};

enum Mode { None, Set, Scale, Add, Force, Skipped, Out, Matrix, Smooth };

struct Request {
    int mode = None;
    std::array<float, 16> v{};
    int len = 0;
};

struct Slot {
    std::atomic<int> mode{None};
    std::array<std::atomic<float>, 16> v;
    std::atomic<int> len{0};
    void* orig = nullptr;
    bool hooked = false;
    bool tried = false;
    int arg = 1;
    bool rowMajor = false;
    bool before = false;
    std::array<float, 2> acc{};
    uintptr_t patched = 0;
    std::array<uint8_t, 64> backup{};
    size_t backupLen = 0;
    Report last;

    Slot() {
        for (auto& x : v) x = 0.f;
    }
};

constexpr size_t ghostMax = 16;
std::array<std::atomic<uintptr_t>, ghostMax> ghostActor;
std::array<double, ghostMax> ghostUntil{};

std::array<Request, count> pending;
std::array<Slot, count> slots;

using Fn = uintptr_t (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);
using FnF = float (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);

void multiply(const float* a, const float* b, float* r) {
    for (int c = 0; c < 4; c++)
        for (int row = 0; row < 4; row++) {
            float sum = 0.f;
            for (int k = 0; k < 4; k++) sum += a[k * 4 + row] * b[c * 4 + k];
            r[c * 4 + row] = sum;
        }
}

void transpose(float* m) {
    for (int r = 0; r < 4; r++)
        for (int c = r + 1; c < 4; c++) std::swap(m[r * 4 + c], m[c * 4 + r]);
}

void applyMatrix(uintptr_t address, Slot& sl) {
    float m[16];
    if (!mem::read(address, m)) return;
    if (sl.rowMajor) transpose(m);

    float mv[3] = {sl.v[0], sl.v[1], sl.v[2]}, sc[3] = {sl.v[3], sl.v[4], sl.v[5]}, rot[3] = {sl.v[6] * 0.0174533f, sl.v[7] * 0.0174533f, sl.v[8] * 0.0174533f};
    float t[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, mv[0], mv[1], mv[2], 1};
    float s[16] = {sc[0], 0, 0, 0, 0, sc[1], 0, 0, 0, 0, sc[2], 0, 0, 0, 0, 1};
    float cx = std::cos(rot[0]), sx = std::sin(rot[0]), cy = std::cos(rot[1]), sy = std::sin(rot[1]), cz = std::cos(rot[2]), sz = std::sin(rot[2]);
    float rx[16] = {1, 0, 0, 0, 0, cx, sx, 0, 0, -sx, cx, 0, 0, 0, 0, 1};
    float ry[16] = {cy, 0, -sy, 0, 0, 1, 0, 0, sy, 0, cy, 0, 0, 0, 0, 1};
    float rz[16] = {cz, sz, 0, 0, -sz, cz, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    float a[16], b[16], c[16], d[16];
    multiply(t, rz, a);
    multiply(a, ry, b);
    multiply(b, rx, c);
    multiply(c, s, a);
    multiply(m, a, d);
    if (sl.rowMajor) transpose(d);
    mem::write(address, d);
}

template <size_t N>
struct Detour {
    static uintptr_t flag(uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d) {
        auto& sl = slots[N];
        if (sl.mode == Force) return sl.v[0] > 0.5f ? 1 : 0;
        if (sl.mode == Skipped) return 0;
        return reinterpret_cast<Fn>(sl.orig)(a, b, c, d);
    }

    static uintptr_t integer(uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d) {
        auto& sl = slots[N];
        if (sl.mode == Force) return uintptr_t(int(sl.v[0]));
        return reinterpret_cast<Fn>(sl.orig)(a, b, c, d);
    }

    static uintptr_t ghost(uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d) {
        auto& sl = slots[N];
        uintptr_t args[4] = {a, b, c, d};
        if (sl.mode == Force && ghosted(args[std::clamp(sl.arg, 0, 3)])) return 0;
        return reinterpret_cast<Fn>(sl.orig)(a, b, c, d);
    }

    static uintptr_t filter(uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d) {
        auto& sl = slots[N];
        uintptr_t r = reinterpret_cast<Fn>(sl.orig)(a, b, c, d);
        return sl.mode == Force && r && ghosted(r) ? 0 : r;
    }

    static uintptr_t skip(uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d) {
        auto& sl = slots[N];
        if (sl.mode == Skipped) return 0;
        return reinterpret_cast<Fn>(sl.orig)(a, b, c, d);
    }

    static float value(uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d) {
        auto& sl = slots[N];
        float r = reinterpret_cast<FnF>(sl.orig)(a, b, c, d);
        switch (sl.mode.load()) {
        case Set: return sl.v[0];
        case Scale: return r * sl.v[0];
        case Add: return r + sl.v[0];
        case Skipped: return 0.f;
        default: return r;
        }
    }

    static uintptr_t out(uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d) {
        auto& sl = slots[N];
        uintptr_t args[4] = {a, b, c, d};
        uintptr_t target = args[std::clamp(sl.arg, 0, 3)];
        int mode = sl.mode.load();
        bool pre = sl.before || mode == Smooth;
        if (pre) modify(sl, target, mode);
        uintptr_t r = reinterpret_cast<Fn>(sl.orig)(a, b, c, d);
        if (!pre) modify(sl, target, mode);
        return r;
    }

    static void modify(Slot& sl, uintptr_t target, int mode) {
        if (mode == Out) {
            for (int k = 0; k < sl.len; k++) mem::write(target + 4 * k, sl.v[k].load());
        } else if (mode == Matrix) {
            applyMatrix(target, sl);
        } else if (mode == Smooth) {
            float cur[2];
            if (!mem::read(target, cur)) return;
            sl.acc[0] += cur[0];
            sl.acc[1] += cur[1];
            float f = std::clamp(sl.v[0].load(), 0.02f, 1.f);
            float o[2] = {sl.acc[0] * f, sl.acc[1] * f};
            sl.acc[0] -= o[0];
            sl.acc[1] -= o[1];
            mem::write(target, o);
        }
    }
};

template <size_t... I>
auto makeTable(std::index_sequence<I...>) {
    struct Row {
        void* flag;
        void* integer;
        void* skip;
        void* value;
        void* out;
        void* ghost;
        void* filter;
    };
    return std::array<Row, sizeof...(I)>{Row{reinterpret_cast<void*>(&Detour<I>::flag), reinterpret_cast<void*>(&Detour<I>::integer), reinterpret_cast<void*>(&Detour<I>::skip),
                                              reinterpret_cast<void*>(&Detour<I>::value), reinterpret_cast<void*>(&Detour<I>::out),
                                              reinterpret_cast<void*>(&Detour<I>::ghost), reinterpret_cast<void*>(&Detour<I>::filter)}...};
}

const auto detours = makeTable(std::make_index_sequence<count>{});

Kind kindOf(size_t i) {
    int o = sigs::offset(std::string(table[i].sig) + ".kind", -1);
    return o >= 0 && o <= int(Kind::Filter) ? Kind(o) : table[i].kind;
}

void restorePatch(Slot& sl) {
    if (!sl.patched) return;
    for (size_t k = 0; k < sl.backupLen; k++) mem::write(sl.patched + k, sl.backup[k]);
    sl.patched = 0;
}

void install(size_t i, Slot& sl, uintptr_t address) {
    sl.tried = true;
    sl.arg = sigs::offset(std::string(table[i].sig) + ".arg", 1);
    sl.rowMajor = sigs::offset(std::string(table[i].sig) + ".rowMajor", 0) == 1;
    sl.before = sigs::offset(std::string(table[i].sig) + ".before", 0) == 1;
    void* target = reinterpret_cast<void*>(address);
    const auto& row = detours[i];
    void* detour = nullptr;
    switch (kindOf(i)) {
    case Kind::Flag: detour = row.flag; break;
    case Kind::Int: detour = row.integer; break;
    case Kind::Skip: detour = row.skip; break;
    case Kind::Value: detour = row.value; break;
    case Kind::Out: detour = row.out; break;
    case Kind::Ghost: detour = row.ghost; break;
    case Kind::Filter: detour = row.filter; break;
    case Kind::Data: return;
    }
    sl.hooked = hook::create(table[i].sig, target, detour, &sl.orig);
    if (sl.hooked) hook::enableAll();
}

void patch(size_t i, Slot& sl, const Request& r, uintptr_t address) {
    (void)i;
    size_t bytes = std::min<size_t>(sizeof(float) * size_t(r.len), sl.backup.size());
    if (!bytes) return;
    if (sl.patched != address) {
        restorePatch(sl);
        for (size_t k = 0; k < bytes; k++) sl.backup[k] = mem::get<uint8_t>(address + k);
        sl.backupLen = bytes;
        sl.patched = address;
    }
    for (int k = 0; k < r.len; k++) mem::write(address + 4 * k, r.v[size_t(k)]);
}

}

const Info& info(Id id) { return table[size_t(id)]; }

std::string sig(Id id) { return table[size_t(id)].sig; }

void begin() {
    for (auto& r : pending) r = Request{};
}

static Request& req(Id id) { return pending[size_t(id)]; }

void set(Id id, float v) {
    auto& r = req(id);
    r.mode = Set;
    r.v[0] = v;
}

void scale(Id id, float m) {
    auto& r = req(id);
    if (r.mode == Scale) m *= r.v[0];
    r.mode = Scale;
    r.v[0] = m;
}

void add(Id id, float a) {
    auto& r = req(id);
    if (r.mode == Add) a += r.v[0];
    r.mode = Add;
    r.v[0] = a;
}

void force(Id id, bool on) {
    auto& r = req(id);
    r.mode = Force;
    r.v[0] = on ? 1.f : 0.f;
}

void setInt(Id id, int v) {
    auto& r = req(id);
    r.mode = Force;
    r.v[0] = float(v);
}

void skip(Id id) { req(id).mode = Skipped; }

void smooth(Id id, float factor) {
    auto& r = req(id);
    r.mode = Smooth;
    r.len = 1;
    r.v[0] = factor;
}

void out(Id id, std::initializer_list<float> values) {
    auto& r = req(id);
    r.mode = Out;
    r.len = int(std::min<size_t>(values.size(), r.v.size()));
    int k = 0;
    for (float v : values)
        if (k < r.len) r.v[size_t(k++)] = v;
}

void transform(Id id, game::Vec3 move, game::Vec3 scale, game::Vec3 rotateDeg) {
    auto& r = req(id);
    if (r.mode == Matrix) {
        r.v[0] += move.x;
        r.v[1] += move.y;
        r.v[2] += move.z;
        r.v[3] *= scale.x;
        r.v[4] *= scale.y;
        r.v[5] *= scale.z;
        r.v[6] += rotateDeg.x;
        r.v[7] += rotateDeg.y;
        r.v[8] += rotateDeg.z;
        return;
    }
    r.mode = Matrix;
    r.len = 9;
    r.v = {move.x, move.y, move.z, scale.x, scale.y, scale.z, rotateDeg.x, rotateDeg.y, rotateDeg.z};
}

void ghost(uintptr_t actor, float seconds) {
    if (!actor) return;
    double until = ui::time() + double(seconds);
    size_t slot = ghostMax;
    for (size_t i = 0; i < ghostMax; i++) {
        uintptr_t cur = ghostActor[i].load();
        if (cur == actor) {
            ghostUntil[i] = until;
            return;
        }
        if (!cur && slot == ghostMax) slot = i;
    }
    if (slot == ghostMax)
        slot = size_t(std::min_element(ghostUntil.begin(), ghostUntil.end()) - ghostUntil.begin());
    ghostUntil[slot] = until;
    ghostActor[slot] = actor;
}

bool ghosted(uintptr_t actor) {
    if (!actor) return false;
    for (auto& g : ghostActor)
        if (g.load(std::memory_order_relaxed) == actor) return true;
    return false;
}

int ghostCount() {
    int n = 0;
    for (auto& g : ghostActor) n += g.load() != 0;
    return n;
}

bool available(Id id) { return game::demo() || sigs::address(table[size_t(id)].sig) != 0; }

Report report(Id id) { return slots[size_t(id)].last; }

void apply() {
    double now = ui::time();
    for (size_t i = 0; i < ghostMax; i++)
        if (ghostActor[i].load() && ghostUntil[i] < now) ghostActor[i] = 0;
    bool demo = game::demo();
    for (size_t i = 0; i < count; i++) {
        auto& r = pending[i];
        auto& sl = slots[i];
        sl.last.requested = r.mode != None;
        sl.last.value = r.v[0];
        sl.last.installed = sl.hooked || sl.patched != 0;

        uintptr_t address = demo ? 0 : sigs::address(table[i].sig);
        if (r.mode == None || !address) {
            sl.mode = None;
            restorePatch(sl);
            continue;
        }

        Kind kind = kindOf(i);
        if (kind == Kind::Data) {
            patch(i, sl, r.mode == Set ? Request{Set, r.v, 1} : r, address);
            continue;
        }
        if (!sl.tried) install(i, sl, address);
        if (!sl.hooked) continue;
        for (size_t k = 0; k < r.v.size(); k++) sl.v[k] = r.v[k];
        sl.len = r.len;
        sl.mode = r.mode;
    }
}

void shutdown() {
    for (auto& g : ghostActor) g = 0;
    for (auto& sl : slots) {
        sl.mode = None;
        restorePatch(sl);
    }
}

}
