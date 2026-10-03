#include "Bg.hpp"
#include "Guard.hpp"
#include "Log.hpp"

#include <windows.h>

#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

namespace bg {

static std::mutex lock;
static std::vector<HANDLE> threads;
static bool closed = false;

// The default stack of a new thread is whatever the host exe asks for, often 1 MB. The shader compiler needs
// more than that for the big post shader, so background work gets a fixed reservation (committed only as used).
static constexpr SIZE_T stackReserve = 8 << 20;

static DWORD WINAPI worker(LPVOID p) {
    std::unique_ptr<std::function<void()>> work(static_cast<std::function<void()>*>(p));
    guard::call("background", *work);
    return 0;
}

static void reap() {
    std::erase_if(threads, [](HANDLE t) {
        if (WaitForSingleObject(t, 0) != WAIT_OBJECT_0) return false;
        CloseHandle(t);
        return true;
    });
}

void run(std::function<void()> work) {
    std::scoped_lock g(lock);
    if (closed) {
        logger::warn("background job refused, the client is unloading");
        return;
    }
    reap();
    auto* job = new std::function<void()>(std::move(work));
    HANDLE t = CreateThread(nullptr, stackReserve, worker, job, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);
    if (t) {
        threads.push_back(t);
        return;
    }
    logger::warn("background thread failed ({}), running inline", GetLastError());
    worker(job);
}

// The dll may only be freed once every worker has left its code; a thread still inside it would crash the game.
bool drain(int timeoutMs) {
    std::vector<HANDLE> left;
    {
        std::scoped_lock g(lock);
        closed = true;
        left.swap(threads);
    }
    ULONGLONG until = GetTickCount64() + ULONGLONG(timeoutMs);
    bool done = true;
    for (HANDLE t : left) {
        ULONGLONG now = GetTickCount64();
        DWORD wait = now < until ? DWORD(until - now) : 0;
        if (WaitForSingleObject(t, wait) != WAIT_OBJECT_0) done = false;
    }
    int running = 0;
    for (HANDLE t : left) {
        if (WaitForSingleObject(t, 0) != WAIT_OBJECT_0) running++;
        CloseHandle(t);
    }
    if (!done) logger::warn("{} background threads still running at unload", running);
    return done;
}

}
