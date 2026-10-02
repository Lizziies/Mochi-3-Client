#include "Bg.hpp"
#include "Guard.hpp"
#include "Log.hpp"

#include <windows.h>

#include <atomic>
#include <memory>

namespace bg {

static std::atomic<int> running{0};

// The default stack of a new thread is whatever the host exe asks for, often 1 MB. The shader compiler needs
// more than that for the big post shader, so background work gets a fixed reservation (committed only as used).
static constexpr SIZE_T stackReserve = 8 << 20;

static DWORD WINAPI worker(LPVOID p) {
    std::unique_ptr<std::function<void()>> work(static_cast<std::function<void()>*>(p));
    guard::call("background", *work);
    running--;
    return 0;
}

void run(std::function<void()> work) {
    running++;
    auto* job = new std::function<void()>(std::move(work));
    HANDLE t = CreateThread(nullptr, stackReserve, worker, job, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);
    if (t) {
        CloseHandle(t);
        return;
    }
    logger::warn("background thread failed ({}), running inline", GetLastError());
    worker(job);
}

// the DLL is freed right after this returns, so a thread that is still inside its code would crash the game
void drain(int timeoutMs) {
    for (int waited = 0; running > 0 && waited < timeoutMs; waited += 20) Sleep(20);
    if (running > 0) logger::warn("{} background threads still running at unload", running.load());
    Sleep(30);
}

}
