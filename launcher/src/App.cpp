#include "App.hpp"
#include "Build.hpp"
#include "Files.hpp"
#include "Game.hpp"
#include "I18n.hpp"
#include "Net.hpp"
#include "Update.hpp"

#include <json.hpp>

#include <commdlg.h>
#include <shellapi.h>

#include <atomic>
#include <cstdio>
#include <mutex>
#include <thread>

using i18n::tr;

namespace app {

namespace {

struct Shared {
    std::mutex lock;
    ui::Phase phase = ui::Phase::Idle;
    float progress = 0.f;
    std::string status;
    std::string latest;
    std::string notes;
    bool updateAvailable = false;
    bool launcherUpdate = false;
    std::optional<update::Release> pending;
    std::string gameVersion;
    bool gameSupported = true;
};

Shared shared;
std::atomic<bool> busy{false};
std::atomic<bool> quit{false};
std::atomic<DWORD> lastInjected{0};
Settings current;

void set(ui::Phase phase, const std::string& status, float progress = 0.f) {
    std::lock_guard g(shared.lock);
    shared.phase = phase;
    shared.status = status;
    shared.progress = progress;
}

void setProgress(float p) {
    std::lock_guard g(shared.lock);
    shared.progress = p;
}

void fail(const std::string& error) { set(ui::Phase::Failed, tr(error.c_str())); }

bool supported(const std::string& version) {
    if (version.empty()) return true;
    auto body = net::get(std::string("https://raw.githubusercontent.com/") + files::narrow(build::repoOwner) + "/" +
                         files::narrow(build::repoName) + "/" + files::narrow(build::repoBranch) + "/sigs/index.json");
    if (!body) return true;
    auto j = nlohmann::json::parse(*body, nullptr, false);
    if (j.is_discarded() || !j.contains("versions")) return true;
    for (auto& v : j["versions"])
        if (v.is_string() && v.get<std::string>() == version) return true;
    return false;
}

void refreshGame() {
    std::string version = game::installedVersion();
    bool ok = supported(version);
    std::lock_guard g(shared.lock);
    shared.gameVersion = version;
    shared.gameSupported = ok;
}

void checkUpdate() {
    auto release = update::latest(current.beta);
    std::lock_guard g(shared.lock);
    if (!release) return;
    shared.pending = release;
    shared.latest = release->tag;
    shared.notes = release->notes;
    shared.updateAvailable = update::newer(release->tag, update::installedTag());
    shared.launcherUpdate = !release->launcherUrl.empty() && update::newer(release->tag, build::version);
}

std::filesystem::path clientDll() {
    if (!current.customDll.empty()) return files::fs::path(files::widen(current.customDll));
    wchar_t self[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    auto beside = files::fs::path(self).parent_path() / L"Mochi.dll";
    if (!files::fs::exists(files::dll()) && files::fs::exists(beside)) {
        std::error_code ec;
        files::fs::copy_file(beside, files::dll(), files::fs::copy_options::overwrite_existing, ec);
    }
    return files::dll();
}

std::optional<update::Release> pending() {
    std::lock_guard g(shared.lock);
    return shared.pending;
}

bool installUpdate() {
    auto release = pending();
    if (!release || !current.customDll.empty()) return true;
    if (!update::newer(release->tag, update::installedTag()) && files::fs::exists(files::dll())) return true;
    set(ui::Phase::Updating, i18n::fmt("Downloading {}", release->tag));
    std::string error;
    if (!update::installDll(*release, setProgress, error)) {
        if (!files::fs::exists(files::dll())) {
            fail(error);
            return false;
        }
        return true;
    }
    std::lock_guard g(shared.lock);
    shared.updateAvailable = false;
    return true;
}

bool connect(DWORD pid) {
    if (game::injected(pid)) {
        lastInjected = pid;
        return true;
    }
    set(ui::Phase::Injecting, tr("Connecting the client"));
    std::string error;
    if (!game::inject(pid, clientDll(), error)) {
        fail(error);
        return false;
    }
    lastInjected = pid;
    return true;
}

std::optional<DWORD> startGame() {
    if (auto pid = game::running()) return pid;
    set(ui::Phase::Starting, tr("Starting Minecraft"));
    if (!game::launch()) {
        fail("Minecraft could not be started");
        return std::nullopt;
    }
    for (int i = 0; i < 180; i++) {
        if (auto pid = game::running()) return pid;
        Sleep(250);
    }
    fail("Minecraft did not start");
    return std::nullopt;
}

void play() {
    set(ui::Phase::Updating, tr("Checking for updates"));
    checkUpdate();
    if (!installUpdate()) return;

    auto pid = startGame();
    if (!pid) return;

    set(ui::Phase::Waiting, tr("Waiting for the game to load"));
    if (!game::waitReady(*pid, 120000)) {
        fail("Minecraft closed before it was ready");
        return;
    }
    if (!connect(*pid)) return;

    set(ui::Phase::Done, tr("Mochi is connected. Have fun!"));
    if (current.closeAfterInject) quit = true;
}

void updateAll() {
    set(ui::Phase::Updating, tr("Checking for updates"));
    checkUpdate();
    if (!installUpdate()) return;
    auto release = pending();
    bool restart = false;
    {
        std::lock_guard g(shared.lock);
        restart = shared.launcherUpdate && release && !release->launcherUrl.empty();
    }
    if (restart) {
        set(ui::Phase::Updating, tr("Updating the launcher"));
        std::string error;
        if (update::swapLauncher(*release, setProgress, error)) {
            quit = true;
            return;
        }
        fail(error);
        return;
    }
    set(ui::Phase::Idle, "");
}

void watch() {
    while (!quit) {
        Sleep(1500);
        if (busy || !current.autoInject) continue;
        auto pid = game::running();
        if (!pid || lastInjected == *pid || game::injected(*pid)) continue;
        busy = true;
        if (game::waitReady(*pid, 120000) && connect(*pid)) {
            set(ui::Phase::Done, tr("Mochi is connected. Have fun!"));
            if (current.closeAfterInject) quit = true;
        } else {
            set(ui::Phase::Idle, "");
        }
        busy = false;
    }
}

void spawn(void (*job)()) {
    if (busy.exchange(true)) return;
    std::thread([job] {
        job();
        busy = false;
    }).detach();
}

}

void init(ui::State& state) {
    i18n::load();
    update::cleanup();
    current = Settings::load();
    state.settings = current;
    std::snprintf(state.dllPath, sizeof(state.dllPath), "%s", current.customDll.c_str());
    state.clientVersion = build::version;

    std::string tag = update::installedTag();
    shared.latest = tag;
    std::thread([] {
        refreshGame();
        checkUpdate();
    }).detach();
    std::thread(watch).detach();
}

void handle(ui::State& state, const ui::Events& ev, HWND window) {
    if (ev.settingsChanged) {
        current = state.settings;
        current.save();
    }
    if (ev.play) spawn(play);
    if (ev.update) spawn(updateAll);
    if (ev.minimize) ShowWindow(window, SW_MINIMIZE);
    if (ev.close) PostMessageW(window, WM_CLOSE, 0, 0);
    if (ev.openLogs) ShellExecuteW(nullptr, L"open", files::log().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (ev.openFolder) ShellExecuteW(nullptr, L"open", files::root().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (ev.browseDll) {
        wchar_t path[MAX_PATH] = {};
        OPENFILENAMEW ofn{sizeof(ofn)};
        ofn.hwndOwner = window;
        ofn.lpstrFilter = L"DLL\0*.dll\0\0";
        ofn.lpstrFile = path;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (GetOpenFileNameW(&ofn)) {
            std::snprintf(state.dllPath, sizeof(state.dllPath), "%s", files::narrow(path).c_str());
            state.settings.customDll = state.dllPath;
            current = state.settings;
            current.save();
        }
    }
}

void sync(ui::State& state) {
    std::lock_guard g(shared.lock);
    state.phase = shared.phase;
    state.progress = shared.progress;
    state.status = shared.status;
    state.latestVersion = shared.latest;
    state.updateAvailable = shared.updateAvailable;
    state.gameVersion = shared.gameVersion;
    state.gameSupported = shared.gameSupported;
    if (!shared.notes.empty()) state.changelog = shared.notes;

    state.versions.clear();
    if (!shared.gameVersion.empty()) state.versions.push_back({shared.gameVersion, false, true, shared.gameSupported, true});
}

bool wantsQuit() { return quit; }

}
