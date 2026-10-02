#pragma once

#include <imgui.h>

namespace gui {

enum class Icon {
    Sword, Crystal, Shield, Bolt, Run, Box, Drop, Swing, Target, Potion, Heart, Spark, Mouse, Timer,
    Keyboard, Gauge, Armor, Compass, Bag, Clock, Layers, Info, Signal, Bell,
    Zoom, Sun, Crosshair, Cube, Camera, Eye, Pickaxe, Palette, Sparkle, Cloud, Tag, Pin,
    Chat, Trophy, Mask, Wrench, Plug, Chip, Chart, Gear, Hex, Server, Game, Grid, Star, Lock,
};

void icon(ImDrawList* dl, Icon kind, ImVec2 center, float radius, ImU32 color);

}
