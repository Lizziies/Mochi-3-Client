#pragma once

#include "Types.hpp"

#include <imgui.h>

#include <functional>
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
void setDemoServer(const std::string& name);
const std::string& demoServer();

void filterChat(std::function<bool(const std::string&)> hide);
bool chatHidden(const std::string& text);

bool ready(unsigned mask);
bool freeCamera(bool on);
void hide(uintptr_t entity, float seconds);
int hidden();
bool has(Domain d);
void lease(unsigned mask, int delta);

void resetCombat();
std::optional<ImVec2> project(const Vec3& p);
bool projectLine(const Vec3& a, const Vec3& b, ImVec2& out0, ImVec2& out1);

}
