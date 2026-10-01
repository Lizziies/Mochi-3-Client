#include "App.hpp"
#include "Build.hpp"
#include "Embedded.hpp"
#include "Files.hpp"
#include "Game.hpp"
#include "I18n.hpp"
#include "Levi.hpp"
#include "Net.hpp"
#include "Update.hpp"
#include "Versions.hpp"

#include <json.hpp>

#include <commdlg.h>
#include <shellapi.h>
#include <shobjidl.h>

#include <algorithm>
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
    bool managerInstalled = false;
    bool managerBusy = false;
    float managerProgress = 0.f;
    std::string managerStatus;
    std::vector<versions::Install> installs;
    std::vector<std::string> sigVersions;
    bool sigsKnown = false;
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

void loadSigIndex() {
    auto body = net::get(std::string("https://raw.githubusercontent.com/") + files::narrow(build::repoOwner) + "/" +
                         files::narrow(build::repoName) + "/" + files::narrow(build::repoBranch) + "/sigs/index.json");
    if (!body) return;
    auto j = nlohmann::json::parse(*body, nullptr, false);
    if (j.is_discarded() || !j.contains("versions")) return;
    std::lock_guard g(shared.lock);
    shared.sigVersions.clear();
    for (auto& v : j["versions"])
        if (v.is_string()) shared.sigVersions.push_back(v.get<std::string>());
    shared.sigsKnown = true;
}

bool supportedLocked(const std::string& version) {
    if (version.empty() || !shared.sigsKnown) return true;
    return std::find(shared.sigVersions.begin(), shared.sigVersions.end(), version) != shared.sigVersions.end();
}

void rescan() {
    std::vector<files::fs::path> extra;
    for (auto& f : current.folders) extra.push_back(files::fs::path(files::widen(f)));
    auto found = versions::scan(extra);
    std::lock_guard g(shared.lock);
    shared.installs = std::move(found);
}

void refreshGame() {
    std::string version = game::installedVersion();
    loadSigIndex();
    std::lock_guard g(shared.lock);
    shared.gameVersion = version;
    shared.gameSupported = supportedLocked(version);
}

void checkUpdate() {
    auto release = update::latest(current.beta);
    std::lock_guard g(shared.lock);
    if (!release) return;
    shared.pending = release;
    shared.latest = release->tag;
    shared.notes = release->notes;
    shared.launcherUpdate = !release->launcherUrl.empty() && update::newer(release->tag, build::version);
    shared.updateAvailable = release->dllUrl.empty() ? shared.launcherUpdate : update::newer(release->tag, update::installedTag());
}

std::filesystem::path clientDll() {
    if (!current.customDll.empty()) return files::fs::path(files::widen(current.customDll));
    std::string error;
    if (!embedded::install(error)) fail(error);
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
    if (!release || release->dllUrl.empty() || !current.customDll.empty()) return true;
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

bool sameFile(const std::wstring& a, const std::string& b) {
    std::error_code ec;
    return !a.empty() && std::filesystem::equivalent(files::fs::path(a), files::fs::path(files::widen(b)), ec);
}

std::optional<DWORD> startGame() {
    if (auto pid = game::running()) {
        if (!current.pinned.empty() && !sameFile(game::runningPath(*pid), current.pinned)) {
            fail("Minecraft is already running with another version. Close it first.");
            return std::nullopt;
        }
        return pid;
    }
    set(ui::Phase::Starting, tr("Starting Minecraft"));
    std::string error = "Minecraft could not be started";
    bool started = current.pinned.empty() ? game::launch() : game::launchExe(files::fs::path(files::widen(current.pinned)), error);
    if (!started) {
        fail(error);
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
        fail(current.pinned.empty() ? "Minecraft closed before it was ready"
                                    : "The selected version closed before it was ready. Pick another version or use the Store one.");
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

void installManager() {
    {
        std::lock_guard g(shared.lock);
        shared.managerBusy = true;
        shared.managerProgress = 0.f;
        shared.managerStatus = tr("Downloading LeviLauncher");
    }
    std::string error;
    bool ok = levi::install(
        [](float p) {
            std::lock_guard g(shared.lock);
            shared.managerProgress = p;
        },
        error);
    {
        std::lock_guard g(shared.lock);
        shared.managerBusy = false;
        shared.managerInstalled = !levi::find().empty();
        shared.managerStatus = ok ? "" : tr(error.c_str());
    }
    if (ok) levi::open();
}

std::optional<files::fs::path> pickFolder(HWND owner) {
    IFileOpenDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) return std::nullopt;
    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
    std::optional<files::fs::path> result;
    if (SUCCEEDED(dialog->Show(owner))) {
        IShellItem* item = nullptr;
        PWSTR name = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item)) && SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &name))) {
            result = files::fs::path(name);
            CoTaskMemFree(name);
        }
        if (item) item->Release();
    }
    dialog->Release();
    return result;
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
        rescan();
        checkUpdate();
    }).detach();
    shared.managerInstalled = !levi::find().empty();
    std::thread(watch).detach();
}

void handle(ui::State& state, const ui::Events& ev, HWND window) {
    if (ev.settingsChanged) {
        current = state.settings;
        current.save();
    }
    if (ev.play) spawn(play);
    if (ev.update) spawn(updateAll);
    if (ev.installManager) {
        static std::atomic<bool> installing{false};
        if (!installing.exchange(true)) std::thread([] { installManager(); installing = false; }).detach();
    }
    if (ev.openManager) levi::open();
    if (ev.pick >= 0) {
        std::string path;
        {
            std::lock_guard g(shared.lock);
            size_t i = size_t(ev.pick);
            if (i >= 1 && i <= shared.installs.size()) path = files::narrow(shared.installs[i - 1].exe.wstring());
        }
        current.pinned = path;
        state.settings.pinned = path;
        current.save();
    }
    if (ev.rescan) std::thread([] { rescan(); }).detach();
    if (ev.addFolder) {
        if (auto folder = pickFolder(window)) {
            std::string path = files::narrow(folder->wstring());
            if (std::find(current.folders.begin(), current.folders.end(), path) == current.folders.end()) current.folders.push_back(path);
            state.settings.folders = current.folders;
            current.save();
            std::thread([] { rescan(); }).detach();
        }
    }
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
    state.managerInstalled = shared.managerInstalled;
    state.managerBusy = shared.managerBusy;
    state.managerProgress = shared.managerProgress;
    state.managerStatus = shared.managerStatus;
    if (!shared.notes.empty()) state.changelog = shared.notes;

    state.versions.clear();
    bool pinnedFound = false;
    for (auto& i : shared.installs)
        if (!current.pinned.empty() && sameFile(i.exe.wstring(), current.pinned)) pinnedFound = true;
    bool storeActive = current.pinned.empty() || !pinnedFound;
    state.versions.push_back({shared.gameVersion, false, !shared.gameVersion.empty(), shared.gameSupported, storeActive, true, {}});
    for (auto& i : shared.installs) {
        bool active = !storeActive && sameFile(i.exe.wstring(), current.pinned);
        state.versions.push_back({i.name, i.preview, true, supportedLocked(i.name), active, false, files::narrow(i.exe.wstring())});
    }
}

bool wantsQuit() { return quit; }

}
