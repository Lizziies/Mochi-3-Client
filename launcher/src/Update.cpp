#include "Update.hpp"
#include "Build.hpp"
#include "Files.hpp"
#include "Net.hpp"

#include <json.hpp>

#include <windows.h>
#include <bcrypt.h>
#include <shellapi.h>

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <vector>

using nlohmann::json;

namespace update {

namespace {

std::string apiBase() {
    return "https://api.github.com/repos/" + files::narrow(build::repoOwner) + "/" + files::narrow(build::repoName);
}

std::vector<int> parts(std::string v) {
    if (!v.empty() && (v[0] == 'v' || v[0] == 'V')) v.erase(0, 1);
    std::vector<int> out;
    std::stringstream ss(v);
    std::string item;
    while (std::getline(ss, item, '.')) out.push_back(std::atoi(item.c_str()));
    return out;
}

std::optional<Release> parse(const json& j) {
    if (!j.is_object() || !j.contains("tag_name")) return std::nullopt;
    Release r;
    r.tag = j.value("tag_name", "");
    r.notes = j.value("body", "");
    for (auto& a : j.value("assets", json::array())) {
        std::string name = a.value("name", "");
        std::string url = a.value("browser_download_url", "");
        if (name == "Monchi.dll") r.dllUrl = url;
        else if (name == "MonchiLauncher.exe") r.launcherUrl = url;
        else if (name == "checksums.txt") r.sumsUrl = url;
    }
    return r;
}

std::string sha256(const std::filesystem::path& p) {
    std::string data = files::read(p);
    BCRYPT_ALG_HANDLE alg = nullptr;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) return {};
    unsigned char digest[32] = {};
    BCryptHash(alg, nullptr, 0, (PUCHAR)data.data(), (ULONG)data.size(), digest, sizeof(digest));
    BCryptCloseAlgorithmProvider(alg, 0);
    static const char* hex = "0123456789abcdef";
    std::string out;
    for (unsigned char b : digest) {
        out += hex[b >> 4];
        out += hex[b & 15];
    }
    return out;
}

std::string expectedHash(const std::string& sums, const std::string& file) {
    std::stringstream ss(sums);
    std::string line;
    while (std::getline(ss, line)) {
        auto at = line.find(file);
        if (at == std::string::npos || at == 0) continue;
        std::string hash = line.substr(0, line.find_first_of(" \t"));
        std::transform(hash.begin(), hash.end(), hash.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        return hash;
    }
    return {};
}

bool fetch(const std::string& url, const std::string& name, const std::string& sums,
           const std::filesystem::path& to, const std::function<void(float)>& progress, std::string& error) {
    if (url.empty()) {
        error = "The release is incomplete";
        return false;
    }
    if (!net::download(url, to, progress)) {
        error = "Download failed";
        return false;
    }
    std::string want = expectedHash(sums, name);
    if (want.empty() || want != sha256(to)) {
        std::error_code ec;
        std::filesystem::remove(to, ec);
        error = "Checksum does not match";
        return false;
    }
    return true;
}

}

std::optional<Release> latest(bool beta) {
    auto body = net::get(apiBase() + (beta ? "/releases?per_page=5" : "/releases/latest"));
    if (!body) return std::nullopt;
    json j = json::parse(*body, nullptr, false);
    if (j.is_discarded()) return std::nullopt;
    if (j.is_array()) return j.empty() ? std::nullopt : parse(j[0]);
    return parse(j);
}

bool newer(const std::string& candidate, const std::string& current) {
    auto a = parts(candidate), b = parts(current);
    size_t n = std::max(a.size(), b.size());
    a.resize(n);
    b.resize(n);
    return std::lexicographical_compare(b.begin(), b.end(), a.begin(), a.end());
}

std::string installedTag() {
    std::string t = files::read(files::installedTag());
    while (!t.empty() && (t.back() == '\n' || t.back() == '\r' || t.back() == ' ')) t.pop_back();
    return t;
}

bool installDll(const Release& r, const std::function<void(float)>& progress, std::string& error) {
    std::string sums = r.sumsUrl.empty() ? std::string() : net::get(r.sumsUrl).value_or("");
    auto tmp = files::bin() / L"Monchi.dll.part";
    if (!fetch(r.dllUrl, "Monchi.dll", sums, tmp, progress, error)) return false;

    if (!MoveFileExW(tmp.c_str(), files::dll().c_str(), MOVEFILE_REPLACE_EXISTING)) {
        std::error_code ec;
        std::filesystem::remove(tmp, ec);
        error = "Close Minecraft to update the client";
        return false;
    }
    files::write(files::installedTag(), r.tag);
    return true;
}

bool swapLauncher(const Release& r, const std::function<void(float)>& progress, std::string& error) {
    std::string sums = r.sumsUrl.empty() ? std::string() : net::get(r.sumsUrl).value_or("");
    auto next = files::root() / L"MonchiLauncher.new.exe";
    if (!fetch(r.launcherUrl, "MonchiLauncher.exe", sums, next, progress, error)) return false;

    wchar_t self[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring old = std::wstring(self) + L".old";
    DeleteFileW(old.c_str());
    if (!MoveFileExW(self, old.c_str(), 0) || !MoveFileExW(next.c_str(), self, 0)) {
        MoveFileExW(old.c_str(), self, MOVEFILE_REPLACE_EXISTING);
        error = "Could not replace the launcher";
        return false;
    }
    ShellExecuteW(nullptr, L"open", self, nullptr, nullptr, SW_SHOWNORMAL);
    return true;
}

void cleanup() {
    wchar_t self[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    DeleteFileW((std::wstring(self) + L".old").c_str());
}

}
