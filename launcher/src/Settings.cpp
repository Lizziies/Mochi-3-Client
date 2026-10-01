#include "Settings.hpp"
#include "Files.hpp"

#include <json.hpp>

using nlohmann::json;

Settings Settings::load() {
    Settings s;
    auto j = json::parse(files::read(files::settings()), nullptr, false);
    if (j.is_discarded() || !j.is_object()) return s;
    s.beta = j.value("beta", s.beta);
    s.autoInject = j.value("autoInject", s.autoInject);
    s.closeAfterInject = j.value("closeAfterInject", s.closeAfterInject);
    s.customDll = j.value("customDll", s.customDll);
    return s;
}

void Settings::save() const {
    json j = {
        {"beta", beta},
        {"autoInject", autoInject},
        {"closeAfterInject", closeAfterInject},
        {"customDll", customDll},
    };
    files::write(files::settings(), j.dump(2));
}
