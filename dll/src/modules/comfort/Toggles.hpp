#pragma once

#include "core/Config.hpp"
#include "gui/Gui.hpp"
#include "gui/Notify.hpp"
#include "hook/Dx.hpp"
#include "hook/GameInput.hpp"
#include "modules/HudModule.hpp"
#include "modules/common/Keys.hpp"
#include "render/Ui.hpp"
#include "sdk/Game.hpp"
#include "sdk/Inject.hpp"

#include <windows.h>

#include <format>

class StickyKey : public HudModule {
public:
    StickyKey(std::string name, std::string desc, int defaultKey, ImVec2 pos, const char* onText, const char* offText)
        : HudModule(std::move(name), std::move(desc), {"input"}, pos), key_(keySetting("gameKey", "Key in game", defaultKey)),
          onText_(textSetting("onText", "Text when on", onText)), offText_(textSetting("offText", "Text when off", offText)) {
        sub("Movement");
        background_.b = true;
    }

    void onKey(KeyEvent& ev) override {
        if (inject::ours() || ev.vk != key_.i || !key_.i) return;
        if (mode() == 1) return;
        if (ev.repeat) {
            if (held_) ev.cancel = true;
            return;
        }
        if (ev.down) {
            if (held_) {
                held_ = false;
                ev.cancel = true;
                inject::key(key_.i, false);
            } else {
                held_ = true;
            }
        } else if (held_) {
            ev.cancel = true;
        }
    }

    void onFrame() override {
        if (mode() == 1) autoUpdate();
        else if (autoHeld_) {
            inject::key(key_.i, false);
            autoHeld_ = false;
        }
        if ((held_ || autoHeld_) && inject::focused()) gameinput::hold(key_.i);
    }

    void onDisable() override {
        if (held_ || autoHeld_) inject::key(key_.i, false);
        held_ = autoHeld_ = false;
    }

    void onRender(ImDrawList* dl) override {
        bool on = held_ || autoHeld_;
        if (!on && !always_.b && !gui::editingHud()) return;
        HudModule::onRender(dl);
    }

protected:
    virtual int mode() const = 0;
    virtual void autoUpdate() {}

    ImVec2 content(ImDrawList* dl, ImVec2 o, float s) override {
        bool on = held_ || autoHeld_;
        return drawText(dl, o, s, i18n::tr((on ? onText_ : offText_).text.c_str()), on ? accentColor() : textColor());
    }

    Setting& key_;
    Setting& onText_;
    Setting& offText_;
    Setting& always_ = toggleSetting("always", "Also show when off", false);
    bool held_ = false;
    bool autoHeld_ = false;
};

class ToggleSprint : public StickyKey {
public:
    ToggleSprint() : StickyKey("Toggle Sprint", "Sprinting stays on until you press the key again.", VK_LCONTROL, {0.005f, 0.925f}, "[Sprint: on]", "[Sprint: off]") {}

protected:
    int mode() const override { return mode_.i; }

    void autoUpdate() override {
        bool forward = input::down(forward_.i);
        if (forward && !autoHeld_ && inject::focused() && !gui::capturesKeyboard()) {
            inject::key(key_.i, true);
            autoHeld_ = true;
        } else if (!forward && autoHeld_) {
            inject::key(key_.i, false);
            autoHeld_ = false;
        }
    }

private:
    Setting& mode_ = choice("mode", "Mode", {"Toggle with the key", "Automatic while walking forward"});
    Setting& forward_ = keySetting("forward", "Forward key", 'W');
};

class ToggleSneak : public StickyKey {
public:
    ToggleSneak() : StickyKey("Toggle Sneak", "Sneaking stays on until you press the sneak key again.", VK_LSHIFT, {0.005f, 0.96f}, "[Sneak: on]", "[Sneak: off]") {}

protected:
    int mode() const override { return 0; }
};

class SlotHotkeys : public Module {
public:
    SlotHotkeys(std::string name, std::string desc, bool command)
        : Module(std::move(name), std::move(desc), Category::Comfort, {"chat"}), command_(command) {
        sub("Chat");
        for (int i = 0; i < slots; i++) {
            std::string n = std::to_string(i + 1);
            keys_[i] = &keySetting("key" + n, "Key " + n, 0);
            texts_[i] = &textSetting("text" + n, command ? "Command " + n : "Text " + n, "");
        }
        chatKey_ = &keySetting("chatKey", "Chat key in game", 'T');
    }

    void onKey(KeyEvent& ev) override {
        if (!ev.down || ev.repeat) return;
        for (int i = 0; i < slots; i++) {
            if (!keys_[i]->i || ev.vk != keys_[i]->i || texts_[i]->text.empty()) continue;
            std::string t = texts_[i]->text;
            if (command_ && t[0] != '/') t = "/" + t;
            inject::say(t, chatKey_->i);
        }
    }

private:
    static constexpr int slots = 6;
    bool command_;
    Setting* keys_[slots]{};
    Setting* texts_[slots]{};
    Setting* chatKey_ = nullptr;
};

class CommandHotkey : public SlotHotkeys {
public:
    CommandHotkey() : SlotHotkeys("Command Hotkey", "Hotkeys for chat commands like /hub. Opens the chat like a player and sends the command.", true) {}
};

class TextHotkey : public SlotHotkeys {
public:
    TextHotkey() : SlotHotkeys("Text Hotkey", "Hotkeys for ready-made chat messages.", false) {}
};

class ProfileHotkeys : public Module {
public:
    ProfileHotkeys()
        : Module("Profile Hotkeys", "Switches between your saved settings profiles on a key.", Category::Comfort, {"cosmetic"}) {
        sub("Profiles");
        for (int i = 0; i < slots; i++) {
            std::string n = std::to_string(i + 1);
            keys_[i] = &keySetting("key" + n, "Key " + n, 0);
            names_[i] = &textSetting("profile" + n, "Profile " + n, "");
        }
    }

    void onKey(KeyEvent& ev) override {
        if (!ev.down || ev.repeat) return;
        for (int i = 0; i < slots; i++) {
            if (!keys_[i]->i || ev.vk != keys_[i]->i || names_[i]->text.empty()) continue;
            auto all = config::profiles();
            if (std::find(all.begin(), all.end(), names_[i]->text) == all.end()) {
                notify::push(i18n::tr("Profile not found"), names_[i]->text, notify::Kind::Warn);
                continue;
            }
            config::switchProfile(names_[i]->text);
            notify::push(i18n::tr("Profile switched"), names_[i]->text, notify::Kind::Ok);
            return;
        }
    }

    void drawSettings() override {
        ImGui::Spacing();
        std::string list;
        for (auto& p : config::profiles()) list += (list.empty() ? "" : ", ") + p;
        ImGui::TextDisabled(i18n::tr("Available profiles: %s"), list.c_str());
        ImGui::TextDisabled(i18n::tr("Active: %s"), config::profile().c_str());
    }

private:
    static constexpr int slots = 4;
    Setting* keys_[slots]{};
    Setting* names_[slots]{};
};
