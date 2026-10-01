#include "Files.hpp"

#include <windows.h>
#include <shlobj.h>

#include <fstream>

namespace files {

static fs::path ensure(const fs::path& p) {
    std::error_code ec;
    fs::create_directories(p, ec);
    return p;
}

fs::path root() {
    static fs::path cached = [] {
        PWSTR local = nullptr;
        fs::path p;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local))) {
            p = fs::path(local) / L"Mochi";
            CoTaskMemFree(local);
        } else {
            p = fs::current_path() / L"MochiData";
        }
        return ensure(p);
    }();
    return cached;
}

fs::path bin() { return ensure(root() / L"bin"); }
fs::path dll() { return bin() / L"Mochi.dll"; }
fs::path installedTag() { return bin() / L"version.txt"; }
fs::path settings() { return root() / L"launcher.json"; }
fs::path log() { return root() / L"logs" / L"latest.log"; }

std::string read(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return in ? std::string(std::istreambuf_iterator<char>(in), {}) : std::string();
}

bool write(const fs::path& p, const std::string& data) {
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    out << data;
    return bool(out);
}

std::string narrow(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), out.data(), n, nullptr, nullptr);
    return out;
}

std::wstring widen(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring out(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), out.data(), n);
    return out;
}

}
