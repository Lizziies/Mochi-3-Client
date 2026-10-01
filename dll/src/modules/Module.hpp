#pragma once

#include "Setting.hpp"
#include "core/Events.hpp"
#include "sdk/Game.hpp"

#include <imgui.h>
#include <json.hpp>

#include <deque>
#include <string>
#include <vector>

enum class Category { Hud, Visual, Pvp, Comfort, Performance, Server, Fun, Client };

const char* categoryName(Category c);

enum class RuleLevel { Allowed, Warn, Block };

class Module {
public:
    Module(std::string name, std::string description, Category category, std::vector<std::string> tags = {},
           std::vector<std::string> sigs = {});
    virtual ~Module() = default;

    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;

    virtual void onEnable() {}
    virtual void onDisable() {}
    virtual void onFrame() {}
    virtual void onRender(ImDrawList*) {}
    virtual void onKey(KeyEvent&) {}
    virtual void onMouse(MouseEvent&) {}
    virtual void onServer(const ServerEvent&) {}
    virtual void drawSettings() {}
    virtual bool isHud() const { return false; }
    virtual bool alwaysOn() const { return false; }
    virtual bool persistent() const { return true; }
    virtual bool defaultEnabled() const { return false; }

    void setEnabled(bool on);
    void toggle() { setEnabled(!enabled_); }
    bool enabled() const { return enabled_; }
    bool userEnabled() const { return wanted_; }

    const std::string& name() const { return name_; }
    const std::string& description() const { return description_; }
    Category category() const { return category_; }
    const std::string& sub() const { return sub_; }
    const std::vector<std::string>& tags() const { return tags_; }
    const std::vector<std::string>& sigs() const { return sigs_; }
    bool hasTag(const std::string& t) const;

    bool available() const { return game::demo() || (missing_.empty() && game::ready(needs_)); }
    const std::vector<std::string>& missingSigs() const { return missing_; }
    void checkSigs();

    RuleLevel rule() const { return rule_; }
    const std::string& ruleNote() const { return ruleNote_; }
    void applyRule(RuleLevel level, std::string note);
    bool optionBlocked(const std::string& option) const;
    void setBlockedOptions(std::vector<std::string> opts) { blockedOptions_ = std::move(opts); }

    std::deque<Setting>& settings() { return settings_; }
    Setting& keybind() { return *key_; }

    bool risky() const { return risky_; }

    nlohmann::json save() const;
    void load(const nlohmann::json& j);

    float anim = 0.f;
    float hover = 0.f;

protected:
    Setting& toggleSetting(std::string id, std::string label, bool def);
    Setting& slider(std::string id, std::string label, float def, float min, float max, const char* fmt = "%.1f");
    Setting& intSlider(std::string id, std::string label, int def, int min, int max);
    Setting& colorSetting(std::string id, std::string label, ImVec4 def);
    Setting& choice(std::string id, std::string label, std::vector<std::string> options, int def = 0);
    Setting& keySetting(std::string id, std::string label, int def = 0);
    Setting& textSetting(std::string id, std::string label, std::string def);

    void markRisky() { risky_ = true; }
    void sub(std::string name) { sub_ = std::move(name); }
    void needs(unsigned mask) { needs_ = mask; }
    void needs(game::Domain d) { needs_ = unsigned(d); }
    void require(unsigned domains, std::vector<std::string> sigs) {
        needs_ = domains;
        sigs_ = std::move(sigs);
    }

private:
    Setting& add(Setting s);

    std::string name_;
    std::string description_;
    Category category_;
    std::string sub_;
    std::vector<std::string> tags_;
    std::vector<std::string> sigs_;
    std::vector<std::string> missing_;
    std::vector<std::string> blockedOptions_;
    std::deque<Setting> settings_;
    Setting* key_ = nullptr;
    unsigned needs_ = 0;
    bool enabled_ = false;
    bool wanted_ = false;
    bool risky_ = false;
    RuleLevel rule_ = RuleLevel::Allowed;
    std::string ruleNote_;
};
