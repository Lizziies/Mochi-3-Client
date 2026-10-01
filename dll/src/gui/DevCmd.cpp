#include "Gui.hpp"
#include "GuiInternal.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"
#include "modules/Manager.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace gui {

// development only: a file named dev.cmd in the data folder drives the menu, one command per line
void pollDevCommands() {
    static const bool dev = std::filesystem::exists(paths::dllDir() / L"Mochi.root");
    static int tick = 0;
    if (!dev || ++tick % 20) return;

    auto file = paths::root() / L"dev.cmd";
    std::error_code ec;
    if (!std::filesystem::exists(file, ec)) return;

    std::ifstream in(file);
    std::vector<std::string> lines;
    for (std::string line; std::getline(in, line);) lines.push_back(line);
    in.close();
    std::filesystem::remove(file, ec);

    for (auto& line : lines) {
        std::istringstream ss(line);
        std::string cmd;
        ss >> cmd;
        std::string rest;
        std::getline(ss, rest);
        if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);

        if (cmd == "open") {
            setOpen(true);
            go(Page::Hub);
        } else if (cmd == "close") {
            setOpen(false);
            setEditingHud(false);
        } else if (cmd == "page") {
            setOpen(true);
            go(rest == "modules" ? Page::Modules : rest == "cosmetics" ? Page::Cosmetics : rest == "settings" ? Page::Settings : Page::Hub);
        } else if (cmd == "settings") {
            setOpen(true);
            go(Page::Settings);
            settingsTab() = std::atoi(rest.c_str());
            restartContentAnim();
        } else if (cmd == "module") {
            if (auto* m = modules::find(rest)) showModule(m);
        } else if (cmd == "hudedit") {
            setEditingHud(true);
        } else if (cmd == "more") {
            setOpen(true);
            go(Page::Modules);
            showMoreModules() = true;
        } else if (cmd == "search") {
            setOpen(true);
            go(Page::Modules);
            std::snprintf(searchText(), 64, "%s", rest.c_str());
        }
        logger::info("dev command: {}", line);
    }
}

}
