// SPDX-License-Identifier: AGPL-3.0-only
// Adapted from Flarial UpdatePlayerHook and Freelook; see vendor/flarial/UPSTREAM.json.
#include "FreeCamera.hpp"
#include "CodePatch.hpp"
#include "Hook.hpp"
#include "core/Guard.hpp"
#include "sig/Sigs.hpp"

#include <atomic>

namespace freecam {
namespace {
using Update = void (*)(void*, void*, void*);
Update original = nullptr;
std::atomic<bool> detached{false};
bool installed = false;
uintptr_t yaw = 0, headYaw = 0;
constexpr std::array<uint8_t, 4> yawStore{0xf3, 0x0f, 0x11, 0x00};
constexpr std::array<uint8_t, 5> headStore{0xf3, 0x44, 0x0f, 0x11, 0x08};
constexpr std::array<uint8_t, 4> yawNops{0x90, 0x90, 0x90, 0x90};
constexpr std::array<uint8_t, 5> headNops{0x90, 0x90, 0x90, 0x90, 0x90};

void update(void* a, void* b, void* c) {
    guard::call("camera player update", [&] {
        if (!detached.load(std::memory_order_acquire)) original(a, b, c);
    });
}
}

bool set(bool active) {
    if (!active) {
        if (yaw) codePatch::replace(yaw, yawNops, yawStore);
        if (headYaw) codePatch::replace(headYaw, headNops, headStore);
        yaw = headYaw = 0;
        detached.store(false, std::memory_order_release);
        return false;
    }
    if (detached.load(std::memory_order_acquire)) return true;
    auto yawAt = sigs::address("CameraYaw");
    auto headAt = sigs::address("CameraHeadYaw");
    if (!yawAt || !headAt) return false;
    if (!installed) {
        auto address = sigs::address("CameraUpdatePlayer");
        if (!address) return false;
        if (!original && !hook::create("CameraUpdatePlayer", reinterpret_cast<void*>(address), update, &original)) return false;
        if (!hook::enableAll()) return false;
        installed = true;
    }
    // Bedrock 1.26's head-angle store includes a REX prefix and is five bytes,
    // whereas the body-angle store is four. Never split either instruction.
    detached.store(true, std::memory_order_release);
    if (!codePatch::replace(yawAt, yawStore, yawNops)) {
        detached.store(false, std::memory_order_release);
        return false;
    }
    if (!codePatch::replace(headAt, headStore, headNops)) {
        codePatch::replace(yawAt, yawNops, yawStore);
        detached.store(false, std::memory_order_release);
        return false;
    }
    yaw = yawAt;
    headYaw = headAt;
    return true;
}
}
