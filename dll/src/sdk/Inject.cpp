#include "Inject.hpp"
#include "core/Log.hpp"
#include "hook/Dx.hpp"

#include <windows.h>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace inject {

namespace {

constexpr ULONG_PTR marker = 0x4D4F4348;

struct Job {
    std::string text;
    int chatKey;
    bool tapOnly = false;
};

std::mutex lock;
std::condition_variable wake;
std::deque<Job> jobs;
std::thread worker;
std::atomic<bool> stopping{false};

bool extended(int vk) {
    switch (vk) {
    case VK_UP:
    case VK_DOWN:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_RCONTROL:
    case VK_RMENU:
        return true;
    default:
        return false;
    }
}

void send(INPUT& in) { SendInput(1, &in, sizeof(INPUT)); }

void unicode(wchar_t ch) {
    for (int up = 0; up < 2; up++) {
        INPUT in{};
        in.type = INPUT_KEYBOARD;
        in.ki.wScan = ch;
        in.ki.dwFlags = KEYEVENTF_UNICODE | (up ? KEYEVENTF_KEYUP : 0);
        in.ki.dwExtraInfo = marker;
        send(in);
    }
}

void run() {
    for (;;) {
        Job job;
        {
            std::unique_lock g(lock);
            wake.wait(g, [] { return stopping || !jobs.empty(); });
            if (stopping) return;
            job = std::move(jobs.front());
            jobs.pop_front();
        }
        if (!focused()) continue;
        tap(job.chatKey);
        if (job.tapOnly) continue;
        Sleep(110);
        auto wide = logger::widen(job.text);
        for (wchar_t c : wide) {
            unicode(c);
            Sleep(6);
        }
        Sleep(70);
        tap(VK_RETURN);
        for (int i = 0; i < 12 && !stopping; i++) Sleep(100);
    }
}

}

bool ours() { return GetMessageExtraInfo() == (LPARAM)marker; }

bool focused() {
    HWND w = dx::window();
    return w && GetForegroundWindow() == w;
}

void key(int vk, bool down) {
    if (!focused()) return;
    INPUT in{};
    in.type = INPUT_KEYBOARD;
    in.ki.wScan = (WORD)MapVirtualKeyW((UINT)vk, MAPVK_VK_TO_VSC);
    in.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP) | (extended(vk) ? KEYEVENTF_EXTENDEDKEY : 0);
    in.ki.dwExtraInfo = marker;
    send(in);
}

void tap(int vk) {
    key(vk, true);
    Sleep(25);
    key(vk, false);
}

void tapLater(int vk) {
    std::scoped_lock g(lock);
    if (jobs.size() >= 8) return;
    jobs.push_back({"", vk, true});
    if (!worker.joinable()) worker = std::thread(run);
    wake.notify_one();
}

void say(const std::string& text, int chatKey) {
    if (text.empty() || text.size() > 256) return;
    std::scoped_lock g(lock);
    if (jobs.size() >= 4) return;
    jobs.push_back({text, chatKey});
    if (!worker.joinable()) worker = std::thread(run);
    wake.notify_one();
}

void shutdown() {
    {
        std::scoped_lock g(lock);
        stopping = true;
    }
    wake.notify_all();
    if (worker.joinable()) worker.join();
}

}
