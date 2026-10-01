#include "Log.hpp"
#include "Paths.hpp"

#include <windows.h>

#include <chrono>
#include <fstream>
#include <mutex>

namespace logger {

static std::ofstream file;
static std::mutex lock;

void open() {
    auto dir = paths::logs();
    std::error_code ec;
    if (std::filesystem::exists(dir / L"latest.log"))
        std::filesystem::rename(dir / L"latest.log", dir / L"previous.log", ec);
    file.open(dir / L"latest.log", std::ios::out | std::ios::trunc);
}

void close() {
    std::scoped_lock g(lock);
    file.close();
}

void write(std::string_view level, std::string_view msg) {
    auto now = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
    auto line = std::format("[{:%H:%M:%S}] [{}] {}\n", now, level, msg);
    std::scoped_lock g(lock);
    if (file.is_open()) {
        file << line;
        file.flush();
    }
    OutputDebugStringA(line.c_str());
}

std::string narrow(std::wstring_view w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), out.data(), n, nullptr, nullptr);
    return out;
}

std::wstring widen(std::string_view s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring out(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), out.data(), n);
    return out;
}

}
