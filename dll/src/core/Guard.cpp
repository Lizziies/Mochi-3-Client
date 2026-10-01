#include "Guard.hpp"
#include "Log.hpp"

#include <windows.h>

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

static LONG CALLBACK net(EXCEPTION_POINTERS* info) {
    auto code = info->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION && code != EXCEPTION_STACK_OVERFLOW)
        return EXCEPTION_CONTINUE_SEARCH;

    HMODULE owner = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       static_cast<LPCWSTR>(info->ExceptionRecord->ExceptionAddress), &owner);
    if (owner == self) {
        logger::error("crash inside mochi at {} (code 0x{:08X})", info->ExceptionRecord->ExceptionAddress,
                      (unsigned)code);
    }
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
