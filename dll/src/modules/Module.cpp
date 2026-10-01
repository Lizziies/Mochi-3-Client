#include "Module.hpp"
#include "core/Config.hpp"
#include "core/Guard.hpp"
#include "core/Log.hpp"
#include "sig/Sigs.hpp"

#include <algorithm>

using nlohmann::json;

const char* categoryName(Category c) {
    switch (c) {
    case Category::Hud: return "HUD";
    case Category::Visual: return "Visuell";
    case Category::Pvp: return "PvP";
    case Category::Comfort: return "Komfort";
    case Category::Performance: return "Performance";
    case Category::Server: return "Server";
    case Category::Fun: return "Spaß";
    case Category::Client: return "Client";
    }
    return "";
}

Module::Module(std::string name, std::string description, Category category, std::vector<std::string> tags,
               std::vector<std::string> sigs)
    : name_(std::move(name)), description_(std::move(description)), category_(category), tags_(std::move(tags)),
      sigs_(std::move(sigs)) {
    key_ = &keySetting("key", "Taste");
}

Setting& Module::add(Setting s) {
    settings_.push_back(std::move(s));
    return settings_.back();
}

Setting& Module::toggleSetting(std::string id, std::string label, bool def) {
    Setting s{std::move(id), std::move(label), SettingType::Bool};
    s.b = def;
    return add(std::move(s));
}

Setting& Module::slider(std::string id, std::string label, float def, float min, float max, const char* fmt) {
    Setting s{std::move(id), std::move(label), SettingType::Float};
    s.f = def;
    s.fmin = min;
    s.fmax = max;
    s.format = fmt;
    return add(std::move(s));
}

Setting& Module::intSlider(std::string id, std::string label, int def, int min, int max) {
    Setting s{std::move(id), std::move(label), SettingType::Int};
    s.i = def;
    s.imin = min;
    s.imax = max;
    return add(std::move(s));
}

Setting& Module::colorSetting(std::string id, std::string label, ImVec4 def) {
    Setting s{std::move(id), std::move(label), SettingType::Color};
    s.color = def;
    return add(std::move(s));
}

Setting& Module::choice(std::string id, std::string label, std::vector<std::string> options, int def) {
    Setting s{std::move(id), std::move(label), SettingType::Choice};
    s.choices = std::move(options);
    s.i = def;
    return add(std::move(s));
}

Setting& Module::keySetting(std::string id, std::string label, int def) {
    Setting s{std::move(id), std::move(label), SettingType::Key};
    s.i = def;
    return add(std::move(s));
}

Setting& Module::textSetting(std::string id, std::string label, std::string def) {
    Setting s{std::move(id), std::move(label), SettingType::Text};
    s.text = std::move(def);
    return add(std::move(s));
}

bool Module::hasTag(const std::string& t) const {
    return std::find(tags_.begin(), tags_.end(), t) != tags_.end();
}

void Module::checkSigs() {
    missing_.clear();
    for (auto& s : sigs_)
        if (!sigs::address(s)) missing_.push_back(s);
    if (anySig_ && missing_.size() < sigs_.size()) missing_.clear();
}

void Module::setEnabled(bool on) {
    wanted_ = on;
    bool effective = on && available() && rule_ != RuleLevel::Block;
    if (alwaysOn()) effective = true;
    if (effective == enabled_) return;

    enabled_ = effective;
    if (needs_) game::lease(needs_, enabled_ ? 1 : -1);
    bool ok = guard::call(name_.c_str(), [&] {
        if (enabled_) onEnable();
        else onDisable();
    });
    if (!ok && enabled_) {
        enabled_ = false;
        wanted_ = false;
        if (needs_) game::lease(needs_, -1);
    }
    config::markDirty();
}

void Module::applyRule(RuleLevel level, std::string note) {
    rule_ = level;
    ruleNote_ = std::move(note);
    setEnabled(wanted_);
}

bool Module::optionBlocked(const std::string& option) const {
    return std::find(blockedOptions_.begin(), blockedOptions_.end(), option) != blockedOptions_.end();
}

json Module::save() const {
    json j;
    j["enabled"] = persistent() && wanted_;
    json s = json::object();
    for (auto& set : settings_) s[set.id] = set.save();
    j["settings"] = s;
    return j;
}

void Module::load(const json& j) {
    if (j.contains("settings") && j["settings"].is_object()) {
        auto& s = j["settings"];
        for (auto& set : settings_)
            if (s.contains(set.id)) set.load(s[set.id]);
    }
    if (j.contains("enabled") && j["enabled"].is_boolean()) setEnabled(j["enabled"].get<bool>());
}
