#include "Config.hpp"
#include "Log.hpp"
#include "Paths.hpp"
#include "gui/Theme.hpp"
#include "modules/Manager.hpp"

#include <json.hpp>

#include <atomic>
#include <fstream>

using nlohmann::json;

namespace config {

static std::string active = "default";
static std::atomic<bool> dirty{false};
static bool loading = false;

static std::filesystem::path fileFor(const std::string& name) {
    return paths::configs() / logger::widen(name + ".json");
}

static json read(const std::filesystem::path& p) {
    std::ifstream in(p);
    if (!in) return json::object();
    auto j = json::parse(in, nullptr, false);
    return j.is_discarded() ? json::object() : j;
}

static void write(const std::filesystem::path& p, const json& j) {
    auto tmp = p;
    tmp += L".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        out << j.dump(2);
    }
    std::error_code ec;
    std::filesystem::rename(tmp, p, ec);
}

static void apply(const json& j) {
    loading = true;
    if (j.contains("theme")) theme::load(j["theme"]);
    auto mods = j.contains("modules") && j["modules"].is_object() ? j["modules"] : json::object();
    for (auto& m : modules::all()) {
        if (mods.contains(m->name())) m->load(mods[m->name()]);
    }
    loading = false;
}

void load() {
    auto settings = read(paths::root() / L"settings.json");
    active = settings.value("profile", "default");
    bool firstRun = !std::filesystem::exists(fileFor(active));
    apply(read(fileFor(active)));
    if (firstRun) {
        for (auto& m : modules::all())
            if (m->defaultEnabled()) m->setEnabled(true);
        save();
    }
    dirty = false;
    logger::info("config '{}' loaded", active);
}

void save() {
    json mods = json::object();
    for (auto& m : modules::all()) mods[m->name()] = m->save();
    write(fileFor(active), {{"theme", theme::save()}, {"modules", mods}});
    write(paths::root() / L"settings.json", {{"profile", active}});
    dirty = false;
}

void saveIfDirty() {
    if (dirty) save();
}

void markDirty() {
    if (!loading) dirty = true;
}

const std::string& profile() { return active; }

std::vector<std::string> profiles() {
    std::vector<std::string> out;
    std::error_code ec;
    for (auto& e : std::filesystem::directory_iterator(paths::configs(), ec)) {
        if (e.path().extension() == L".json") out.push_back(logger::narrow(e.path().stem().wstring()));
    }
    if (out.empty()) out.push_back(active);
    return out;
}

void switchProfile(const std::string& name) {
    if (name.empty() || name == active) return;
    save();
    active = name;
    auto j = read(fileFor(name));
    if (!j.empty()) apply(j);
    theme::applyStyle();
    save();
}

void deleteProfile(const std::string& name) {
    if (name == active) return;
    std::error_code ec;
    std::filesystem::remove(fileFor(name), ec);
}

}
