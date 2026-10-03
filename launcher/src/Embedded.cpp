#include "Embedded.hpp"
#include "Build.hpp"
#include "Files.hpp"

#include "../res/resource.h"

#include <windows.h>

#include <cstdint>
#include <cstring>
#include <string_view>

namespace embedded {

namespace {

std::string_view resource(int id) {
    HRSRC found = FindResourceW(nullptr, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!found) return {};
    HGLOBAL loaded = LoadResource(nullptr, found);
    if (!loaded) return {};
    return {static_cast<const char*>(LockResource(loaded)), SizeofResource(nullptr, found)};
}

std::string fingerprint(std::string_view a, std::string_view b, std::string_view c) {
    uint64_t h = 1469598103934665603ull;
    for (std::string_view part : {a, b, c})
        for (unsigned char c : part) h = (h ^ c) * 1099511628211ull;
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%s-%016llx", build::version, static_cast<unsigned long long>(h));
    return buf;
}

bool safe(const std::string& rel) {
    if (rel.empty() || rel[0] == '/' || rel.find("..") != std::string::npos || rel.find(':') != std::string::npos || rel.find('\\') != std::string::npos) return false;
    return rel.size() > 5 && (rel.ends_with(".json") || rel.ends_with(".png"));
}

template <class T>
bool take(std::string_view& in, T& out) {
    if (in.size() < sizeof(T)) return false;
    std::memcpy(&out, in.data(), sizeof(T));
    in.remove_prefix(sizeof(T));
    return true;
}

void unpack(std::string_view pack) {
    uint32_t count = 0;
    if (pack.size() < 4 || std::memcmp(pack.data(), "MCOS", 4) != 0) return;
    pack.remove_prefix(4);
    if (!take(pack, count)) return;
    auto root = files::root() / L"cosmetics";
    for (uint32_t i = 0; i < count; i++) {
        uint16_t len = 0;
        uint32_t size = 0;
        if (!take(pack, len) || pack.size() < len) return;
        std::string rel(pack.substr(0, len));
        pack.remove_prefix(len);
        if (!take(pack, size) || pack.size() < size) return;
        std::string_view data = pack.substr(0, size);
        pack.remove_prefix(size);
        if (!safe(rel)) continue;
        auto path = root / files::fs::path(files::widen(rel));
        std::error_code ec;
        files::fs::create_directories(path.parent_path(), ec);
        files::write(path, std::string(data));
    }
}

// written beside and moved over, so a file the game still has loaded is never left half written
bool place(std::string_view data, const files::fs::path& to) {
    auto part = to;
    part += L".part";
    std::error_code ec;
    if (!files::write(part, std::string(data))) return false;
    if (MoveFileExW(part.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING)) return true;
    files::fs::remove(part, ec);
    return false;
}

}

bool present() { return !resource(IDR_CLIENT).empty(); }

bool install(std::string& error) {
    auto dll = resource(IDR_CLIENT);
    if (dll.empty()) return true;
    auto pack = resource(IDR_COSMETICS);
    auto core = resource(IDR_CORE);

    auto marker = files::bin() / L"embedded.txt";
    std::string want = fingerprint(dll, pack, core);
    std::error_code ec;
    if (files::read(marker) == want && files::fs::exists(files::dll(), ec)) return true;

    if (!place(dll, files::dll())) {
        error = "Close Minecraft to update the client";
        return false;
    }
    if (!core.empty() && !place(core, files::core())) {
        error = "Close Minecraft to update the client";
        return false;
    }
    unpack(pack);
    files::write(files::installedTag(), build::version);
    files::write(marker, want);
    return true;
}

}
