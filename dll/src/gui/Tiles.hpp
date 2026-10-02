#pragma once

#include "Icons.hpp"
#include "modules/Module.hpp"

#include <string>
#include <vector>

namespace gui {

enum class Section { Pvp, Hud, Visual, Utility, Performance, Server, Extras };

constexpr int sectionCount = 7;

struct Tile {
    std::string name;
    std::string blurb;
    Section section = Section::Pvp;
    Icon icon = Icon::Gear;
    std::vector<Module*> members;
    bool group = false;

    int enabled() const;
    int usable() const;
    bool hud() const;
    bool risky() const;
};

const char* sectionName(Section s);
Icon sectionIcon(Section s);

const std::vector<Tile>& tiles();
const Tile* tileOf(const Module& m);
Icon iconOf(const Module& m);

}
