#include "Guard.hpp"
#include "Log.hpp"

#include <windows.h>

#include <atomic>
#include <format>
#include <mutex>
#include <optional>

namespace guard {

static std::mutex lock;
static std::optional<Fault> lastFault;
static thread_local bool failed = false;
static PVOID netHandle = nullptr;
static HMODULE self = nullptr;

static void store(const char* where, std::string what) {
    logger::error("fault in {}: {}", where, what);
    std::scoped_lock g(lock);
    lastFault = Fault{where, std::move(what)};
}

void report(const char* where, const char* what) {
    failed = true;
    store(where, what);
}

#ifdef _MSC_VER
static int filter(unsigned code, const char* where) {
    store(where, std::format("exception 0x{:08X}", code));
    return EXCEPTION_EXECUTE_HANDLER;
}

bool run(const char* where, void (*fn)(void*), void* ctx) {
    failed = false;
    __try {
        fn(ctx);
    } __except (filter(GetExceptionCode(), where)) {
        return false;
    }
    return !failed;
}
#else
bool run(const char*, void (*fn)(void*), void* ctx) {
    failed = false;
    fn(ctx);
    return !failed;
}
#endif

const Fault* last() {
    std::scoped_lock g(lock);
    return lastFault ? &*lastFault : nullptr;
}

void clearLast() {
    std::scoped_lock g(lock);
    lastFault.reset();
}

static std::atomic<int> reported{0};

// Faults inside the game or a system DLL often start in our code. Unwinding is not reliable from a vectored
// handler, so the stack is scanned for values that point into our image; offsets map to symbols with objdump.
static std::string callers(uintptr_t sp) {
    if (!self) return {};
    auto base = reinterpret_cast<uintptr_t>(self);
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    uintptr_t end = base + nt->OptionalHeader.SizeOfImage;
    auto* tib = reinterpret_cast<NT_TIB*>(NtCurrentTeb());
    auto top = reinterpret_cast<uintptr_t>(tib->StackBase);
    std::string out;
    int found = 0;
    for (uintptr_t p = sp & ~uintptr_t(7); p + 8 <= top && p < sp + 0x4000 && found < 10; p += 8) {
        uintptr_t v = *reinterpret_cast<uintptr_t*>(p);
        if (v <= base + 0x1000 || v >= end) continue;
        out += std::format("{}mochi+0x{:X}", found ? " " : "", v - base);
        found++;
    }
    return out;
}

static LONG CALLBACK net(EXCEPTION_POINTERS* info) {
    auto code = info->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION && code != EXCEPTION_STACK_OVERFLOW)
        return EXCEPTION_CONTINUE_SEARCH;

    HMODULE owner = nullptr;
    auto at = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(at), &owner);
    std::string trail = callers(info->ContextRecord->Rsp);
    if (owner != self && trail.empty()) return EXCEPTION_CONTINUE_SEARCH;
    if (++reported > 4) return EXCEPTION_CONTINUE_SEARCH;

    wchar_t path[MAX_PATH] = L"?";
    if (owner) GetModuleFileNameW(owner, path, MAX_PATH);
    std::wstring name = path;
    name = name.substr(name.find_last_of(L"\\/") + 1);
    logger::error("fault 0x{:08X} at {}+0x{:X}, access {} 0x{:X}, called from {}", (unsigned)code,
                  logger::narrow(name), at - reinterpret_cast<uintptr_t>(owner),
                  info->ExceptionRecord->ExceptionInformation[0] ? "write" : "read",
                  info->ExceptionRecord->ExceptionInformation[1], trail.empty() ? "-" : trail);
    return EXCEPTION_CONTINUE_SEARCH;
}

void installNet() {
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&installNet), &self);
    netHandle = AddVectoredExceptionHandler(1, net);
}

void removeNet() {
    if (netHandle) RemoveVectoredExceptionHandler(netHandle);
    netHandle = nullptr;
}

}
