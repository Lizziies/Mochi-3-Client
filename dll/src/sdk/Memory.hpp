#pragma once

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <cstring>

namespace mem {

inline std::atomic<uint32_t> frameStamp{1};

inline void nextFrame() { frameStamp.fetch_add(1, std::memory_order_relaxed); }

// Most writes go to the game's own heap objects, which are writable anyway. Asking once per page and frame is
// one syscall instead of two VirtualProtect calls (each flushing the TLB) for every single write.
inline bool writable(uintptr_t address, size_t size) {
    struct Entry {
        uintptr_t page;
        uint32_t stamp;
        bool ok;
    };
    thread_local Entry cache[8]{};
    uintptr_t page = address & ~uintptr_t(0xFFF);
    if (((address + size - 1) & ~uintptr_t(0xFFF)) != page) return false;
    uint32_t now = frameStamp.load(std::memory_order_relaxed);
    Entry& e = cache[(page >> 12) & 7];
    if (e.page == page && e.stamp == now) return e.ok;
    MEMORY_BASIC_INFORMATION mbi{};
    bool ok = VirtualQuery(reinterpret_cast<void*>(page), &mbi, sizeof(mbi)) && mbi.State == MEM_COMMIT &&
              (mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)) && !(mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS));
    e = {page, now, ok};
    return ok;
}

template <class T>
bool read(uintptr_t address, T& out) {
    if (address < 0x10000) return false;
    SIZE_T got = 0;
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address), &out, sizeof(T), &got) && got == sizeof(T);
}

inline bool readBytes(uintptr_t address, void* out, size_t size) {
    if (address < 0x10000) return false;
    SIZE_T got = 0;
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address), out, size, &got) && got == size;
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
    if (writable(address, sizeof(T))) {
        std::memcpy(reinterpret_cast<void*>(address), &value, sizeof(T));
        return true;
    }
    DWORD old = 0;
    if (!VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), PAGE_EXECUTE_READWRITE, &old)) return false;
    std::memcpy(reinterpret_cast<void*>(address), &value, sizeof(T));
    VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), old, &old);
    return true;
}

}
