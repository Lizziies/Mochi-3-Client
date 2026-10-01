#pragma once

#include "Types.hpp"

#include <imgui.h>

#include <optional>
#include <string>
#include <vector>

namespace game {

class Provider {
public:
    virtual ~Provider() = default;
    virtual unsigned supports() const = 0;
    virtual bool derived() const = 0;
    virtual void update(State& s, std::vector<Event>& events) = 0;
    virtual void use(unsigned) {}
};

void init();
void update();
void shutdown();

const State& state();
const std::vector<Event>& events();

bool demo();
void setDemo(bool on);

bool ready(unsigned mask);
bool has(Domain d);
void lease(unsigned mask, int delta);

void resetCombat();
void resetCombo();
std::optional<ImVec2> project(const Vec3& p);

}
