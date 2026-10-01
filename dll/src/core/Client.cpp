#include "I18n.hpp"
#include "Client.hpp"
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

static void teardown() {
    logger::info("unloading");
    config::save();
    input::uninstall();
    hook::disableAll();
    Sleep(300);
    modules::shutdown();
    tweaks::restore();
    dx::uninstall();
    hook::shutdown();
    guard::removeNet();
    logger::info("bye");
    logger::close();
}

static DWORD WINAPI mainThread(LPVOID) {
    guard::call("boot", boot);

    int ticks = 0;
    while (!leaving) {
        Sleep(50);
        if (++ticks % 100 == 0) config::saveIfDirty();
    }

    teardown();
    FreeLibraryAndExitThread(self, 0);
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
