#include "Tiles.hpp"
#include "I18n.hpp"
#include "core/Log.hpp"
#include "modules/Manager.hpp"

#include <algorithm>
#include <set>

namespace gui {

namespace {

struct Spec {
    const char* name;
    const char* blurb;
    Section section;
    Icon icon;
    std::vector<const char*> members;
};

Spec single(const char* name, Section section, Icon icon) { return {name, "", section, icon, {name}}; }

const std::vector<Spec>& specs() {
    using S = Section;
    static const std::vector<Spec> list = {
        single("Crystal Optimizer", S::Pvp, Icon::Crystal),
        single("Block Hit", S::Pvp, Icon::Shield),
        single("Low Latency", S::Pvp, Icon::Bolt),
        {"Sprint & Sneak", "Toggle sprint and sneak, straight into the game's input", S::Pvp, Icon::Run, {"Toggle Sprint", "Toggle Sneak"}},
        single("Hitbox", S::Pvp, Icon::Box),
        single("Hurt Color", S::Pvp, Icon::Drop),
        single("Animations", S::Pvp, Icon::Swing),
        {"Combat Info", "Reach, combo, hit ping, target and fight stats", S::Pvp, Icon::Target,
         {"Reach Counter", "Combo Counter", "Hit Ping", "Target HUD", "Opponent Reach", "Hit Counter", "Hit Info", "Session Stats",
          "Cooldown Indicator", "Bow Charge"}},
        {"Item Counters", "Pots, totems, arrows and any item you pick", S::Pvp, Icon::Potion,
         {"Pot Counter", "Totem Counter", "Arrow Counter", "Item Counter"}},
        {"Hit Feedback", "Markers, sounds, particles and effects when you hit", S::Pvp, Icon::Spark,
         {"Hit Marker", "Hit Sound", "Hit Effects", "Particle Multiplier", "Damage Indicator", "Kill Effects", "Totem Pop",
          "Insta Hurt Animation"}},
        single("Low Health Indicator", S::Pvp, Icon::Heart),
        {"Aim & Input", "Movement keys, sensitivity, clicks and hotbar keys", S::Pvp, Icon::Mouse,
         {"Null Movement", "Modern Keybind Handling", "Sens Multiplier", "Bow Sensitivity", "Snap Look", "CPS Limiter",
          "Disable Mouse Wheel", "Hotbar Keys"}},
        {"Timing Tweaks", "Faster inventory and item use. Banned on many servers.", S::Pvp, Icon::Timer,
         {"Faster Inventory", "Item Use Delay Fix", "Kill Cleanup"}},

        single("Keystrokes", S::Hud, Icon::Keyboard),
        single("CPS", S::Hud, Icon::Mouse),
        single("FPS", S::Hud, Icon::Gauge),
        single("Armor HUD", S::Hud, Icon::Armor),
        single("Potion HUD", S::Hud, Icon::Potion),
        {"Player Info", "Coordinates, direction, speed, health and XP", S::Hud, Icon::Compass,
         {"Coordinates", "Direction HUD", "Speed Display", "Health Display", "Experience Info", "Fall Predictor", "Look Angles",
          "Day Counter"}},
        {"Inventory HUD", "Inventory, armor bar, held item, paperdoll and hunger", S::Hud, Icon::Bag,
         {"Inventory Viewer", "Hotbar Armor", "Held Item", "Paperdoll", "Durability Warning", "Better Hunger Bar"}},
        {"Clock & Timers", "Clock, stopwatch, session time and pomodoro", S::Hud, Icon::Clock,
         {"Clock", "Stopwatch", "Session Timer", "Pomodoro"}},
        {"Game HUD", "Scoreboard, tab list, GUI scale and the movable vanilla HUD", S::Hud, Icon::Layers,
         {"Scoreboard", "Tab List", "GUI Scale", "Movable Hotbar", "Movable Title", "Movable Bossbar", "Hotbar Animation"}},
        {"World Info", "What you look at, break progress, TNT and light levels", S::Hud, Icon::Info,
         {"Waila", "Break Progress", "TNT Timer", "Entity Counter", "Light Overlay", "Chunk Border", "Death Logger"}},
        {"Info Displays", "Ping, server, packs, memory and watermark", S::Hud, Icon::Signal,
         {"Ping Counter", "Server Display", "IP Display", "Pack Display", "Memory", "Stats HUD", "Mouse Strokes", "Watermark"}},
        single("Subtitles", S::Hud, Icon::Bell),

        single("Zoom", S::Visual, Icon::Zoom),
        single("Fullbright", S::Visual, Icon::Sun),
        single("Custom Crosshair", S::Visual, Icon::Crosshair),
        single("Block Outline", S::Visual, Icon::Cube),
        {"Camera", "Field of view, dynamic FOV, freelook and perspective", S::Visual, Icon::Camera,
         {"FOV Changer", "Java Dynamic FOV", "Freelook", "Auto Perspective", "Cinematic Camera"}},
        {"Clean View", "No hurt cam, no bobbing, low fire and a calmer screen", S::Visual, Icon::Eye,
         {"No Hurt Cam", "No View Bobbing", "Minimal View Bobbing", "Low Fire", "Hide Hand", "Smooth Sneak"}},
        {"Hand & Items", "View model, left hand, item physics and glint", S::Visual, Icon::Pickaxe,
         {"View Model", "Left Hand", "Item Physics", "Glint Color"}},
        {"Screen Filters", "Color, contrast, sharpen, blur and motion blur", S::Visual, Icon::Palette,
         {"Saturation / Hue", "Brightness / Contrast", "Screen Tint", "Sharpen", "Color Filter", "Night Shift", "Motion Blur", "Blur",
          "Depth of Field", "Black Bars"}},
        single("Shader Packs", S::Visual, Icon::Sparkle),
        {"World Look", "Time, weather, sky, fog and water, only for you", S::Visual, Icon::Cloud,
         {"Time Changer", "Weather Changer", "Environment Changer", "Fog Color", "Water Color"}},
        {"Nametags", "Nametag colors, your own nametag and health above heads", S::Visual, Icon::Tag,
         {"Nametag Modifier", "Third Person Nametag", "Health Above Head"}},
        {"Waypoints & Trails", "Waypoints and arrow trails", S::Visual, Icon::Pin, {"Waypoints", "Arrow Trail"}},

        {"Chat", "Better chat, hotkeys, logger, friend alerts and nick", S::Utility, Icon::Chat,
         {"Better Chat", "Command Hotkey", "Text Hotkey", "Message Logger", "Player Notifier", "Nick"}},
        {"Inventory", "Inventory lock and Java inventory hotkeys", S::Utility, Icon::Bag, {"Inventory Lock", "Java Inventory Hotkeys"}},
        single("Auto GG", S::Utility, Icon::Trophy),
        single("Streamer Mode", S::Utility, Icon::Mask),
        single("Screenshot+", S::Utility, Icon::Camera),
        {"Tools", "Skin saver, pack changer, profile and gamemode hotkeys, scripts", S::Utility, Icon::Wrench,
         {"Skin Stealer", "Pack Changer", "Profile Hotkeys", "Gamemode Hotkeys", "Config Sharing", "Lua Scripts"}},
        {"Integrations", "Discord, music, Mumble and Mochi Online", S::Utility, Icon::Plug,
         {"Discord Rich Presence", "Music", "Mumble Link", "Mochi Online"}},

        single("Performance Lock", S::Performance, Icon::Chip),
        single("Frame Limiter", S::Performance, Icon::Gauge),
        single("Render Options", S::Performance, Icon::Layers),
        {"Diagnostics", "Latency, lag analyzer, network monitor and debug menu", S::Performance, Icon::Chart,
         {"Latency Meter", "Lag Analyzer", "Network Monitor", "Debug Menu", "Background Load", "Mouse Sync", "Game Support"}},
        {"System", "Windows tweaks while you play and an automatic profile", S::Performance, Icon::Gear, {"System Boost", "Auto Profile"}},

        {"The Hive", "Requeue, map avoider, stats and leaderboard", S::Server, Icon::Hex, {"Hive Utils", "Hive Stats", "Hive Leaderboard"}},
        single("Zeqa Utils", S::Server, Icon::Server),
        {"Match Tools", "Server profiles and a match summary", S::Server, Icon::Trophy, {"Server Profiles", "Match Summary"}},

        {"Mini Games", "Snake, Flappy Heart and Block Game for the queue", S::Extras, Icon::Game, {"Snake", "Flappy Heart", "Block Game"}},
        {"Fun", "Pet, petals, DVD screen, deepfry and upside down", S::Extras, Icon::Sparkle,
         {"Pet", "Petals", "DVD Screen", "Deepfry", "Upside Down"}},
        single("20-20-20", S::Extras, Icon::Eye),
    };
    return list;
}

Section sectionFor(Category c) {
    switch (c) {
    case Category::Hud: return Section::Hud;
    case Category::Visual: return Section::Visual;
    case Category::Pvp: return Section::Pvp;
    case Category::Comfort: return Section::Utility;
    case Category::Performance: return Section::Performance;
    case Category::Server: return Section::Server;
    default: return Section::Extras;
    }
}

std::vector<Tile> build() {
    std::vector<Tile> out;
    std::set<const Module*> placed;
    for (auto& spec : specs()) {
        Tile t{spec.name, spec.blurb, spec.section, spec.icon, {}, spec.members.size() > 1};
        for (auto* name : spec.members) {
            Module* m = modules::find(name);
            if (!m) {
                logger::warn("menu: tile '{}' lists unknown module '{}'", spec.name, name);
                continue;
            }
            if (!placed.insert(m).second) continue;
            t.members.push_back(m);
        }
        if (!t.members.empty()) out.push_back(std::move(t));
    }
    int loose = 0;
    for (auto& m : modules::all()) {
        if (m->category() == Category::Client || placed.count(m.get())) continue;
        out.push_back({m->name(), "", sectionFor(m->category()), Icon::Gear, {m.get()}, false});
        loose++;
    }
    if (loose) logger::info("menu: {} modules have no tile of their own yet and are shown alone", loose);
    std::stable_sort(out.begin(), out.end(), [](const Tile& a, const Tile& b) { return int(a.section) < int(b.section); });
    return out;
}

bool locked(const Module& m) { return !m.available() || m.rule() == RuleLevel::Block; }

}

int Tile::enabled() const {
    int n = 0;
    for (auto* m : members)
        if (m->userEnabled() && !locked(*m)) n++;
    return n;
}

int Tile::usable() const {
    int n = 0;
    for (auto* m : members)
        if (!locked(*m)) n++;
    return n;
}

bool Tile::hud() const {
    return std::any_of(members.begin(), members.end(), [](Module* m) { return m->isHud(); });
}

bool Tile::risky() const {
    return std::any_of(members.begin(), members.end(), [](Module* m) { return m->risky() || m->rule() == RuleLevel::Warn; });
}

const char* sectionName(Section s) {
    static const char* names[] = {"PvP", "HUD", "Visual", "Utility", "Performance", "Server", "Extras"};
    return i18n::tr(names[int(s)]);
}

Icon sectionIcon(Section s) {
    static const Icon icons[] = {Icon::Sword, Icon::Layers, Icon::Eye, Icon::Wrench, Icon::Bolt, Icon::Server, Icon::Game};
    return icons[int(s)];
}

const std::vector<Tile>& tiles() {
    static const std::vector<Tile> list = build();
    return list;
}

const Tile* tileOf(const Module& m) {
    for (auto& t : tiles())
        for (auto* x : t.members)
            if (x == &m) return &t;
    return nullptr;
}

Icon iconOf(const Module& m) {
    const Tile* t = tileOf(m);
    return t ? t->icon : Icon::Gear;
}

}
