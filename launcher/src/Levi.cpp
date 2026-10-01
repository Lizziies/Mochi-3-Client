#include "Levi.hpp"
#include "Files.hpp"
#include "Net.hpp"

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

namespace levi {

namespace {

constexpr const char* url = "https://github.com/LiteLDev/LeviLauncher/releases/latest/download/LeviLauncher.exe";

std::filesystem::path own() { return files::fs::path(files::root() / L"tools" / L"LeviLauncher.exe"); }

std::filesystem::path under(const KNOWNFOLDERID& id, const wchar_t* sub) {
    PWSTR base = nullptr;
    if (FAILED(SHGetKnownFolderPath(id, 0, nullptr, &base))) return {};
    std::filesystem::path p = std::filesystem::path(base) / sub;
    CoTaskMemFree(base);
    return p;
}

bool looksLikeExe(const std::filesystem::path& p) {
    std::string head = files::read(p);
    return head.size() > (1u << 20) && head[0] == 'M' && head[1] == 'Z';
}

}

std::filesystem::path find() {
    std::error_code ec;
    if (std::filesystem::exists(own(), ec)) return own();
    for (auto& candidate : {under(FOLDERID_LocalAppData, L"Programs\\LeviLauncher\\LeviLauncher.exe"),
                            under(FOLDERID_ProgramFiles, L"LeviLauncher\\LeviLauncher.exe")})
        if (!candidate.empty() && std::filesystem::exists(candidate, ec)) return candidate;
    return {};
}

bool install(const std::function<void(float)>& progress, std::string& error) {
    std::error_code ec;
    std::filesystem::create_directories(own().parent_path(), ec);
    auto part = own();
    part += L".part";
    if (!net::download(url, part, progress)) {
        error = "Download failed";
        return false;
    }
    if (!looksLikeExe(part)) {
        std::filesystem::remove(part, ec);
        error = "The downloaded file is not valid";
        return false;
    }
    std::filesystem::remove(own(), ec);
    std::filesystem::rename(part, own(), ec);
    if (ec) {
        error = "Could not save LeviLauncher";
        return false;
    }
    return true;
}

bool open() {
    auto exe = find();
    if (exe.empty()) return false;
    auto dir = exe.parent_path();
    return reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", exe.c_str(), nullptr, dir.c_str(), SW_SHOWNORMAL)) > 32;
}

}
