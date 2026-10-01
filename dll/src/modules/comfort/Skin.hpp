#pragma once

#include "core/Paths.hpp"
#include "gui/Notify.hpp"
#include "modules/Module.hpp"
#include "modules/common/Needs.hpp"
#include "modules/common/Png.hpp"
#include "sdk/Game.hpp"

#include <cctype>
#include <filesystem>

class SkinStealer : public Module {
public:
    SkinStealer()
        : Module("Skin Stealer", "Press a key while you look at a player to save their skin as a PNG file on your PC. Nothing is sent, your own skin stays as it is.",
                 Category::Comfort, {"cosmetic"}) {
        sub("Packs");
        require(need::target, need::sigs({"Target", "TargetSkin"}));
    }

    void onKey(KeyEvent& ev) override {
        if (!ev.down || ev.repeat || !key_.i || ev.vk != key_.i) return;
        auto& t = game::state().target;
        if (t.kind != game::Target::Kind::Entity || !t.isPlayer || t.skinSize <= 0 || t.skin.size() != size_t(t.skinSize) * size_t(t.skinSize)) {
            notify::push(i18n::tr("Skin Stealer"), i18n::tr("Look at a player first."), notify::Kind::Warn);
            return;
        }
        auto path = paths::root() / "skins" / (clean(t.name) + ".png");
        for (int n = 2; std::filesystem::exists(path); n++) path = paths::root() / "skins" / (clean(t.name) + std::format(" ({}).png", n));
        if (png::write(path, t.skinSize, t.skinSize, t.skin)) {
            last_ = path.string();
            if (copy_.b) ImGui::SetClipboardText(last_.c_str());
            notify::push(i18n::tr("Skin saved"), path.filename().string(), notify::Kind::Ok);
        } else {
            notify::push(i18n::tr("Skin Stealer"), i18n::tr("Could not save the file."), notify::Kind::Error);
        }
    }

    void drawSettings() override {
        ImGui::Spacing();
        ImGui::TextDisabled("%s", i18n::fmt("Folder: {}", (paths::root() / "skins").string()).c_str());
        if (!last_.empty()) ImGui::TextDisabled("%s", i18n::fmt("Last file: {}", last_).c_str());
    }

private:
    static std::string clean(const std::string& name) {
        std::string out;
        for (char c : name) out += std::isalnum((unsigned char)c) || c == '_' || c == '-' ? c : '_';
        return out.empty() ? "player" : out;
    }

    Setting& key_ = keySetting("saveKey", "Save key", 0);
    Setting& copy_ = toggleSetting("copy", "Copy the file path afterwards", false);
    std::string last_;
};
