#pragma once

#include "core/Events.hpp"

#include <windows.h>

#include <cstdint>

namespace input {

bool install(HWND window);
void uninstall();

struct Ours {
    Ours();
    ~Ours();
};

bool gameplay();
void syncCursor(bool menuOpen);
void releaseHeld();

bool down(int vk);
int cps(MouseButton button);
int64_t lastClickQpc();
int64_t lastMoveQpc();

void consumeMotion(int& dx, int& dy);

}
