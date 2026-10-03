#include "I18n.hpp"
#include "Client.hpp"
#include "Bg.hpp"
#include "Build.hpp"
#include "Config.hpp"
#include "Guard.hpp"
#include "Log.hpp"
#include "Paths.hpp"
#include "hook/Dx.hpp"
#include "hook/Hook.hpp"
#include "hook/Input.hpp"
#include "gui/Notify.hpp"
#include "hook/Net.hpp"
#include "modules/Manager.hpp"
#include "modules/post/PostFx.hpp"
#include "sdk/Explore.hpp"
#include "server/Rules.hpp"
#include "sig/Sigs.hpp"
#include "system/Tweaks.hpp"

#include <atomic>

namespace client {

static HMODULE self = nullptr;
static std::atomic<bool> leaving{false};

static void boot() {
    paths::init(self);
    i18n::load();
    logger::open();
    guard::installNet();
    logger::info("{} {} loading", build::name, build::version);

    if (!hook::init()) return;

    sigs::init();
    rules::init();
    modules::init();
    config::load();

    if (!dx::install()) {
        logger::error("renderer hooks failed, nothing to draw on");
    }
    net::install();
    hook::enableAll();

    for (int i = 0; i < 600 && !dx::window() && !leaving; i++) Sleep(50);
    if (dx::window()) input::install(dx::window());

    notify::push(i18n::tr("Mochi loaded"), i18n::tr("Right Shift opens the menu."), notify::Kind::Ok, 6.f);
    logger::info("ready");
}

// returns false when a background job is still running; the dll then has to stay loaded
static bool teardown() {
    logger::info("unloading");
    explore::stop();
    config::save();
    input::uninstall();
    hook::disableAll();
    dx::unhookTables();
    Sleep(300);
    net::waitIdle(6000);
    modules::shutdown();
    bool drained = bg::drain(8000);
    if (drained) post::releaseCompiled();
    tweaks::restore();
    dx::uninstall();
    hook::shutdown();
    guard::removeNet();
    if (drained) logger::info("bye");
    else logger::warn("unload incomplete, staying loaded until the game closes");
    logger::close();
    return drained;
}

static DWORD WINAPI mainThread(LPVOID) {
    guard::call("boot", boot);

    while (!leaving) Sleep(50);

    if (teardown()) FreeLibraryAndExitThread(self, 0);
    ExitThread(0);
    return 0;
}

void start(HMODULE module) {
    self = module;
    DisableThreadLibraryCalls(module);
    HANDLE t = CreateThread(nullptr, 0, mainThread, nullptr, 0, nullptr);
    if (t) CloseHandle(t);
}

void requestUnload() { leaving = true; }
bool unloading() { return leaving; }
HMODULE module() { return self; }

}
