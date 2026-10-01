#include "Providers.hpp"
#include "hook/Input.hpp"

#include <algorithm>
#include <cmath>
#include <random>

namespace game {

namespace {

constexpr unsigned all = unsigned(Domain::Player) | unsigned(Domain::Inventory) | unsigned(Domain::Effects) | unsigned(Domain::Target) |
                         unsigned(Domain::World) | unsigned(Domain::Combat) | unsigned(Domain::Chat) | unsigned(Domain::Scoreboard) |
                         unsigned(Domain::Tab) | unsigned(Domain::Camera);

class Demo : public Provider {
public:
    unsigned supports() const override { return all; }
    bool derived() const override { return false; }

    void update(State& s, std::vector<Event>& ev) override {
        double dt = s.dt;
        s.inWorld = true;
        t_ += dt;
        if (!init_) setup(s);

        movement(s, dt);
        fight(s, ev, dt);
        survival(s, dt);
        world(s, dt);
        target(s);
        chatter(s, ev);
        effects(s, dt);
        scoreboard(s);
    }

private:
    static Item item(const char* name, int count, int maxDamage = 0, int aux = 0, bool glint = false) {
        Item i;
        i.name = name;
        i.count = count;
        i.maxDamage = maxDamage;
        i.aux = aux;
        i.enchanted = glint;
        return i;
    }

    void setup(State& s) {
        init_ = true;
        auto& p = s.player;
        p.name = "MochiSpieler";
        p.pos = {128.5f, 64.f, -42.5f};
        p.hotbar[0] = item("diamond_sword", 1, 1561, 0, true);
        p.hotbar[1] = item("bow", 1, 384, 0, true);
        p.hotbar[2] = item("golden_apple", 8);
        p.hotbar[3] = item("ender_pearl", 12);
        p.hotbar[4] = item("splash_potion", 24, 0, 21);
        p.hotbar[5] = item("diamond_pickaxe", 1, 1561);
        p.hotbar[6] = item("oak_planks", 64);
        p.hotbar[7] = item("cooked_beef", 32);
        p.hotbar[8] = item("water_bucket", 1);
        p.armor[0] = item("diamond_helmet", 1, 363, 0, true);
        p.armor[1] = item("diamond_chestplate", 1, 528, 0, true);
        p.armor[2] = item("diamond_leggings", 1, 495, 0, true);
        p.armor[3] = item("diamond_boots", 1, 429, 0, true);
        p.offhand = item("totem_of_undying", 2);
        p.main.assign(27, Item{});
        p.main[0] = item("arrow", 48);
        p.main[1] = item("totem_of_undying", 3);
        p.main[2] = item("splash_potion", 16, 0, 21);
        p.main[3] = item("iron_ingot", 34);
        p.main[4] = item("diamond", 9);
        p.main[5] = item("golden_apple", 5);
        p.main[6] = item("ender_pearl", 4);
        p.level = 17;
        p.fov = 70.f;
        s.world.packs = {"Mochi Pack", "Vanilla"};
        s.world.players = 11;
        s.world.entities = 41;
        s.world.biome = "plains";
        s.world.time = 1000;
        s.world.day = 12;
        s.player.maxHealth = 20.f;
        s.scoreboard.title = "Mochi Wars";
        for (int i = 0; i < 12; i++) {
            static const char* names[] = {"Luna", "Max", "Kiki", "Noah", "Mia", "Finn", "Lea", "Tim", "Emma", "Ben", "Zoe", "Paul"};
            TabEntry e;
            e.name = names[i];
            e.ping = 20 + (i * 17) % 90;
            s.tab.push_back(e);
        }
        opponentHp_ = 20.f;
    }

    void movement(State& s, double dt) {
        auto& p = s.player;
        float k = float(t_);
        sprint_ = std::fmod(k, 24.f) < 14.f;
        sneak_ = std::fmod(k, 40.f) > 36.f;
        float speed = sneak_ ? 1.3f : sprint_ ? 5.6f : 4.3f;
        float heading = k * 0.18f;
        Vec3 v{std::cos(heading) * speed, 0.f, std::sin(heading) * speed};
        bool jump = std::fmod(k, 3.f) < 0.45f;
        v.y = jump ? (std::fmod(k, 3.f) < 0.22f ? 6.f : -6.f) : 0.f;
        p.vel = v;
        p.pos.x += v.x * float(dt);
        p.pos.z += v.z * float(dt);
        p.pos.y = 64.f + (jump ? 1.1f * std::sin(std::fmod(k, 3.f) / 0.45f * 3.1416f) : 0.f);
        p.onGround = !jump;
        p.sprinting = sprint_;
        p.sneaking = sneak_;
        p.eyeHeight = sneak_ ? 1.27f : 1.62f;
        p.yaw = std::fmod(heading * 57.2958f + 90.f, 360.f);
        if (p.yaw > 180.f) p.yaw -= 360.f;
        p.pitch = 14.f * std::sin(k * 0.6f);
        p.slot = int(std::fmod(k / 6.f, 9.f));
        p.dimension = 0;
        p.view = View::First;
    }

    void fight(State& s, std::vector<Event>& ev, double dt) {
        auto& p = s.player;
        double phase = std::fmod(t_, 16.0);
        fighting_ = phase > 4.0 && phase < 12.0;
        p.usingItem = false;
        p.useProgress = 0.f;
        if (phase > 13.0 && phase < 14.2) {
            p.usingItem = true;
            p.useProgress = float((phase - 13.0) / 1.0);
            if (p.useProgress > 1.f) p.useProgress = 1.f;
            drawing_ = true;
        } else if (drawing_) {
            drawing_ = false;
            Event e{EventKind::BowRelease};
            e.value = 1.f;
            ev.push_back(e);
        }

        if (respawnAt_ > 0.0) {
            if (t_ >= respawnAt_) {
                respawnAt_ = 0.0;
                p.health = 20.f;
                ev.push_back({EventKind::Respawn});
            }
            return;
        }
        if (!fighting_) {
            p.blocking = false;
            return;
        }

        swingTimer_ -= dt;
        if (swingTimer_ <= 0.0) {
            swingTimer_ = 0.09 + dist_(rng_) * 0.1;
            ev.push_back({EventKind::Swing});
            if (dist_(rng_) < 0.72) {
                Event e{EventKind::Hit};
                e.reach = 2.2f + dist_(rng_) * 0.95f;
                e.crit = dist_(rng_) < 0.2;
                e.value = e.crit ? 9.f : 6.f;
                e.text = "Gegner";
                ev.push_back(e);
                opponentHp_ -= e.value * 0.5f;
                auto& sword = p.hotbar[0];
                sword.damage = std::min(sword.maxDamage - 1, sword.damage + 1);
                if (opponentHp_ <= 0.f) {
                    Event k{EventKind::Kill};
                    k.text = "Gegner";
                    ev.push_back(k);
                    opponentHp_ = 20.f;
                }
            }
        }

        hurtTimer_ -= dt;
        if (hurtTimer_ <= 0.0) {
            hurtTimer_ = 0.5 + dist_(rng_) * 0.5;
            if (dist_(rng_) < 0.45) {
                Event e{EventKind::Hurt};
                e.value = 1.5f + dist_(rng_) * 3.f;
                e.reach = 2.3f + dist_(rng_) * 0.9f;
                e.text = "Gegner";
                ev.push_back(e);
                p.health -= e.value;
                for (auto& a : p.armor) a.damage = std::min(a.maxDamage - 1, a.damage + (dist_(rng_) < 0.3 ? 1 : 0));
                if (p.health <= 0.f) {
                    p.health = 0.f;
                    ev.push_back({EventKind::Death});
                    respawnAt_ = t_ + 2.5;
                } else if (p.health < 5.f && p.offhand.count > 0 && dist_(rng_) < 0.4) {
                    p.health = 8.f;
                    Event tp{EventKind::TotemPop};
                    ev.push_back(tp);
                    p.offhand.count--;
                    if (p.offhand.count == 0) p.offhand.count = 2;
                }
            }
        }
        p.blocking = std::fmod(t_, 3.0) < 0.5;
    }

    void survival(State& s, double dt) {
        auto& p = s.player;
        if (!fighting_ && p.health > 0.f && p.health < 20.f) p.health = std::min(20.f, p.health + float(dt) * 0.8f);
        p.absorption = std::fmod(t_, 30.0) < 10.0 ? 4.f : 0.f;
        p.hunger = 20.f - std::fmod(float(t_) * 0.15f, 8.f);
        p.saturation = std::max(0.f, 5.f - std::fmod(float(t_) * 0.3f, 6.f));
        p.xp = std::fmod(float(t_) * 0.03f, 1.f);
        p.air = 300;
        p.onFire = false;
    }

    void world(State& s, double dt) {
        auto& w = s.world;
        timeAcc_ += dt * 20.0;
        w.time = int(std::fmod(1000.0 + timeAcc_, 24000.0));
        w.day = 12 + int((1000.0 + timeAcc_) / 24000.0);
        w.raining = std::fmod(t_, 90.0) > 60.0;
        w.thundering = std::fmod(t_, 180.0) > 150.0;
        w.ping = 38 + int(10.0 * std::sin(t_ * 0.8) + 4.0 * std::sin(t_ * 3.1));
        w.entities = 41 + int(6.0 * std::sin(t_ * 0.2));
        static const char* biomes[] = {"plains", "forest", "desert", "taiga"};
        w.biome = biomes[int(t_ / 45.0) % 4];
    }

    void target(State& s) {
        auto& t = s.target;
        double phase = std::fmod(t_, 16.0);
        if (phase > 4.0 && phase < 12.0) {
            t.kind = Target::Kind::Entity;
            t.name = "Gegner";
            t.isPlayer = true;
            t.distance = 2.4f + 0.6f * std::sin(float(t_) * 3.f);
            t.health = std::max(0.f, opponentHp_);
            t.maxHealth = 20.f;
            t.pos = {s.player.pos.x + 2.f, s.player.pos.y, s.player.pos.z};
        } else if (phase > 14.5) {
            t.kind = Target::Kind::Block;
            t.name = "stone";
            t.blockX = int(s.player.pos.x) + 1;
            t.blockY = 63;
            t.blockZ = int(s.player.pos.z);
            t.distance = 3.f;
            t.breakProgress = float(std::fmod(t_ * 0.7, 1.0));
        } else {
            t.kind = Target::Kind::None;
            t.breakProgress = 0.f;
        }
    }

    void chatter(State& s, std::vector<Event>& ev) {
        if (t_ < nextChat_) return;
        static const char* lines[] = {"<Luna> gg", "<Max> wer hat die Perle?", "§eDas Spiel beginnt in 5 Sekunden", "<Kiki> nice kill",
                                      "§6Runde 3 von 5 startet", "<Noah> lag?", "§aMochi Wars: Du hast gewonnen!"};
        Event e{EventKind::Chat};
        e.text = lines[chatIdx_++ % 7];
        ev.push_back(e);
        nextChat_ = t_ + 4.0 + dist_(rng_) * 4.0;
        (void)s;
    }

    void effects(State& s, double) {
        auto& list = s.player.effects;
        list.clear();
        auto make = [&](const char* id, int amp, float total, double period, double offset, bool good, uint32_t color) {
            float left = float(total - std::fmod(t_ + offset, period));
            if (left <= 0.f) return;
            Effect e;
            e.id = id;
            e.amplifier = amp;
            e.total = total;
            e.seconds = left;
            e.good = good;
            e.color = color;
            list.push_back(e);
        };
        make("speed", 1, 180.f, 190.0, 0.0, true, 0x7CAFC6);
        make("strength", 0, 90.f, 120.0, 20.0, true, 0x932423);
        make("regeneration", 1, 25.f, 40.0, 5.0, true, 0xCD5CAB);
        make("fire_resistance", 0, 60.f, 100.0, 30.0, true, 0xE49A3A);
        make("slowness", 0, 12.f, 70.0, 40.0, false, 0x5A6C81);
    }

    void scoreboard(State& s) {
        s.scoreboard.lines.clear();
        s.scoreboard.lines.push_back({"Kills", s.combat.kills});
        s.scoreboard.lines.push_back({"Tode", s.combat.deaths});
        s.scoreboard.lines.push_back({"Spieler", s.world.players});
        s.scoreboard.lines.push_back({"Runde", 3});
        s.scoreboard.lines.push_back({"mochi.example", 0});
    }

    std::mt19937 rng_{1234};
    std::uniform_real_distribution<double> dist_{0.0, 1.0};
    bool init_ = false;
    double t_ = 0.0;
    double timeAcc_ = 0.0;
    double swingTimer_ = 0.0;
    double hurtTimer_ = 0.0;
    double respawnAt_ = 0.0;
    double nextChat_ = 2.0;
    int chatIdx_ = 0;
    float opponentHp_ = 20.f;
    bool fighting_ = false;
    bool drawing_ = false;
    bool sprint_ = false;
    bool sneak_ = false;
};

}

std::unique_ptr<Provider> makeDemo() { return std::make_unique<Demo>(); }

}
