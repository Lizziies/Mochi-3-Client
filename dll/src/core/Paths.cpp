#include "Paths.hpp"

#include <windows.h>
#include <shlobj.h>

namespace paths {

static fs::path rootDir;
static fs::path moduleDir;

static fs::path ensure(const fs::path& p) {
    std::error_code ec;
    fs::create_directories(p, ec);
    return p;
}

void init(void* module) {
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(static_cast<HMODULE>(module), buf, MAX_PATH);
    moduleDir = fs::path(buf).parent_path();

    PWSTR local = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local))) {
        rootDir = fs::path(local) / L"Mochi";
        CoTaskMemFree(local);
    } else {
        rootDir = moduleDir / L"MochiData";
    }
    ensure(rootDir);
}

const fs::path& root() { return rootDir; }
const fs::path& dllDir() { return moduleDir; }
fs::path configs() { return ensure(rootDir / L"configs"); }
fs::path logs() { return ensure(rootDir / L"logs"); }
fs::path cache() { return ensure(rootDir / L"cache"); }
fs::path scripts() { return ensure(rootDir / L"scripts"); }

}
