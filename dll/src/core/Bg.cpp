#include "Bg.hpp"
#include "Guard.hpp"
#include "Log.hpp"

#include <windows.h>

#include <atomic>
#include <thread>

namespace bg {

static std::atomic<int> running{0};

void run(std::function<void()> work) {
    running++;
    std::thread([work = std::move(work)] {
        guard::call("background", work);
        running--;
    }).detach();
}

// the DLL is freed right after this returns, so a thread that is still inside its code would crash the game
void drain(int timeoutMs) {
    for (int waited = 0; running > 0 && waited < timeoutMs; waited += 20) Sleep(20);
    if (running > 0) logger::warn("{} background threads still running at unload", running.load());
    Sleep(30);
}

}
