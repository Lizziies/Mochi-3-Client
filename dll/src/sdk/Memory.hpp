#pragma once

#include <windows.h>

#include <cstdint>
#include <cstring>

namespace mem {

template <class T>
bool read(uintptr_t address, T& out) {
    if (address < 0x10000) return false;
    SIZE_T got = 0;
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address), &out, sizeof(T), &got) && got == sizeof(T);
}

template <class T>
T get(uintptr_t address, T fallback = T{}) {
    T v{};
    return read(address, v) ? v : fallback;
}

inline uintptr_t pointer(uintptr_t address) { return get<uintptr_t>(address, 0); }

template <class T>
bool write(uintptr_t address, const T& value) {
    if (address < 0x10000) return false;
    DWORD old = 0;
    if (!VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), PAGE_EXECUTE_READWRITE, &old)) return false;
    std::memcpy(reinterpret_cast<void*>(address), &value, sizeof(T));
    VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), old, &old);
    return true;
}

}
