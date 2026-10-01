#include "core/Guard.hpp"
#include "Discord.hpp"

#include <windows.h>
#include <json.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <thread>

using nlohmann::json;

namespace discord {

namespace {

std::mutex lock;
std::condition_variable wake;
std::thread worker;
std::string clientId;
Presence wanted;
bool stopping = false;
std::atomic<int> state{int(Status::Off)};
HANDLE pipe = INVALID_HANDLE_VALUE;

bool writeAll(const std::string& bytes) {
    DWORD written = 0;
    return WriteFile(pipe, bytes.data(), DWORD(bytes.size()), &written, nullptr) && written == bytes.size();
}

bool readFrame(std::string& body, int timeoutMs) {
    auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    for (;;) {
        DWORD avail = 0;
        if (!PeekNamedPipe(pipe, nullptr, 0, nullptr, &avail, nullptr)) return false;
        if (avail >= 8) break;
        if (std::chrono::steady_clock::now() > until) return false;
        Sleep(20);
    }
    uint32_t head[2];
    DWORD got = 0;
    if (!ReadFile(pipe, head, 8, &got, nullptr) || got != 8 || head[1] > (1u << 20)) return false;
    body.assign(head[1], '\0');
    DWORD total = 0;
    while (total < head[1]) {
        DWORD n = 0;
        if (!ReadFile(pipe, body.data() + total, head[1] - total, &n, nullptr) || !n) return false;
        total += n;
    }
    return true;
}

void closePipe() {
    if (pipe != INVALID_HANDLE_VALUE) CloseHandle(pipe);
    pipe = INVALID_HANDLE_VALUE;
}

bool connect() {
    for (int i = 0; i < 10; i++) {
        std::wstring name = L"\\\\.\\pipe\\discord-ipc-" + std::to_wstring(i);
        pipe = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (pipe == INVALID_HANDLE_VALUE) continue;
        if (!writeAll(frame(0, json{{"v", 1}, {"client_id", clientId}}.dump()))) {
            closePipe();
            continue;
        }
        std::string reply;
        if (readFrame(reply, 2500)) return true;
        closePipe();
    }
    return false;
}

void run() {
    int nonce = 0;
    Presence sent;
    bool haveSent = false;
    auto lastSend = std::chrono::steady_clock::now();
    for (;;) {
        Presence now;
        {
            std::unique_lock g(lock);
            wake.wait_for(g, std::chrono::milliseconds(pipe == INVALID_HANDLE_VALUE ? 4000 : 700), [] { return stopping; });
            if (stopping) break;
            now = wanted;
        }
        if (pipe == INVALID_HANDLE_VALUE) {
            state = int(Status::Searching);
            if (!connect()) continue;
            state = int(Status::Connected);
            haveSent = false;
        }
        auto tick = std::chrono::steady_clock::now();
        if (haveSent && now == sent && tick - lastSend < std::chrono::seconds(30)) continue;
        std::string reply;
        if (!writeAll(frame(1, activityJson(now, GetCurrentProcessId(), ++nonce))) || !readFrame(reply, 2500)) {
            closePipe();
            state = int(Status::Searching);
            continue;
        }
        sent = now;
        haveSent = true;
        lastSend = tick;
    }
    if (pipe != INVALID_HANDLE_VALUE) {
        Presence none;
        writeAll(frame(1, activityJson(none, GetCurrentProcessId(), ++nonce)));
        std::string reply;
        readFrame(reply, 300);
        closePipe();
    }
    state = int(Status::Off);
}

}

std::string frame(uint32_t opcode, const std::string& body) {
    std::string out(8, '\0');
    uint32_t len = uint32_t(body.size());
    std::memcpy(out.data(), &opcode, 4);
    std::memcpy(out.data() + 4, &len, 4);
    return out + body;
}

std::string activityJson(const Presence& p, unsigned long pid, int nonce) {
    json args = {{"pid", pid}};
    if (p.active) {
        json activity = json::object();
        if (!p.details.empty()) activity["details"] = p.details;
        if (!p.state.empty()) activity["state"] = p.state;
        if (p.start > 0) activity["timestamps"] = {{"start", p.start}};
        json assets = json::object();
        if (!p.largeImage.empty()) assets["large_image"] = p.largeImage;
        if (!p.largeText.empty()) assets["large_text"] = p.largeText;
        if (!p.smallImage.empty()) assets["small_image"] = p.smallImage;
        if (!p.smallText.empty()) assets["small_text"] = p.smallText;
        if (!assets.empty()) activity["assets"] = assets;
        args["activity"] = activity;
    }
    return json{{"cmd", "SET_ACTIVITY"}, {"args", args}, {"nonce", std::to_string(nonce)}}.dump();
}

void start(const std::string& appId) {
    stop();
    if (appId.empty()) return;
    {
        std::scoped_lock g(lock);
        clientId = appId;
        stopping = false;
    }
    state = int(Status::Searching);
    worker = std::thread([] { guard::call("discord", run); });
}

void stop() {
    {
        std::scoped_lock g(lock);
        stopping = true;
    }
    wake.notify_all();
    if (worker.joinable()) worker.join();
    state = int(Status::Off);
}

void set(const Presence& p) {
    std::scoped_lock g(lock);
    if (wanted == p) return;
    wanted = p;
}

Status status() { return Status(state.load()); }

}
