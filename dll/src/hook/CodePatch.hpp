// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include "sdk/Memory.hpp"
#include <array>
#include <cstring>

namespace codePatch {
template <size_t N>
bool replace(uintptr_t at, const std::array<uint8_t, N>& expected, const std::array<uint8_t, N>& bytes) {
    std::array<uint8_t, N> current{};
    if (!mem::read(at, current) || current != expected) return false;
    DWORD protection = 0;
    if (!VirtualProtect(reinterpret_cast<void*>(at), bytes.size(), PAGE_EXECUTE_READWRITE, &protection)) return false;
    std::memcpy(reinterpret_cast<void*>(at), bytes.data(), bytes.size());
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(at), bytes.size());
    DWORD ignored = 0;
    VirtualProtect(reinterpret_cast<void*>(at), bytes.size(), protection, &ignored);
    return true;
}
}
