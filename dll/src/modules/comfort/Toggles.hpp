#pragma once

#include "core/Config.hpp"
#include "gui/Gui.hpp"
#include "gui/Notify.hpp"
#include "hook/Dx.hpp"
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
        : HudModule(std::move(name), std::move(desc), {"input"}, pos), key_(keySetting("gameKey", "Taste im Spiel", defaultKey)),
          onText_(textSetting("onText", "Text wenn aktiv", onText)), offText_(textSetting("offText", "Text wenn aus", offText)) {
        sub("Bewegung");
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
        if (held_ && !inject::focused()) held_ = false;
        if (mode() == 1) autoUpdate();
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
        return drawText(dl, o, s, on ? onText_.text : offText_.text, on ? accentColor() : textColor());
    }

    Setting& key_;
    Setting& onText_;
    Setting& offText_;
    Setting& always_ = toggleSetting("always", "Auch anzeigen, wenn aus", false);
    bool held_ = false;
    bool autoHeld_ = false;
};

class ToggleSprint : public StickyKey {
public:
    ToggleSprint() : StickyKey("Toggle Sprint", "Sprinten bleibt an, bis du die Sprint-Taste noch einmal drückst, oder läuft automatisch beim Vorwärtsgehen.", VK_LCONTROL, {0.01f, 0.90f}, "[Sprint: an]", "[Sprint: aus]") {}

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
    Setting& mode_ = choice("mode", "Modus", {"Taste umschalten", "Automatisch beim Vorwärtsgehen"});
    Setting& forward_ = keySetting("forward", "Vorwärts-Taste", 'W');
};

class ToggleSneak : public StickyKey {
public:
    ToggleSneak() : StickyKey("Toggle Sneak", "Schleichen bleibt an, bis du die Schleich-Taste noch einmal drückst.", VK_LSHIFT, {0.01f, 0.94f}, "[Schleichen: an]", "[Schleichen: aus]") {}

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
            keys_[i] = &keySetting("key" + n, "Taste " + n, 0);
            texts_[i] = &textSetting("text" + n, command ? "Befehl " + n : "Text " + n, "");
        }
        chatKey_ = &keySetting("chatKey", "Chat-Taste im Spiel", 'T');
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
    CommandHotkey() : SlotHotkeys("Command Hotkey", "Tastenkürzel für Chat-Befehle wie /hub. Öffnet den Chat wie ein Spieler und sendet den Befehl.", true) {}
};

class TextHotkey : public SlotHotkeys {
public:
    TextHotkey() : SlotHotkeys("Text Hotkey", "Tastenkürzel für fertige Chat-Nachrichten.", false) {}
};

class ProfileHotkeys : public Module {
public:
    ProfileHotkeys()
        : Module("Profile Hotkeys", "Wechselt per Taste zwischen deinen gespeicherten Einstellungs-Profilen.", Category::Comfort, {"cosmetic"}) {
        sub("Profile");
        for (int i = 0; i < slots; i++) {
            std::string n = std::to_string(i + 1);
            keys_[i] = &keySetting("key" + n, "Taste " + n, 0);
            names_[i] = &textSetting("profile" + n, "Profil " + n, "");
        }
    }

    void onKey(KeyEvent& ev) override {
        if (!ev.down || ev.repeat) return;
        for (int i = 0; i < slots; i++) {
            if (!keys_[i]->i || ev.vk != keys_[i]->i || names_[i]->text.empty()) continue;
            auto all = config::profiles();
            if (std::find(all.begin(), all.end(), names_[i]->text) == all.end()) {
                notify::push("Profil nicht gefunden", names_[i]->text, notify::Kind::Warn);
                continue;
            }
            config::switchProfile(names_[i]->text);
            notify::push("Profil gewechselt", names_[i]->text, notify::Kind::Ok);
            return;
        }
    }

    void drawSettings() override {
        ImGui::Spacing();
        std::string list;
        for (auto& p : config::profiles()) list += (list.empty() ? "" : ", ") + p;
        ImGui::TextDisabled("Vorhandene Profile: %s", list.c_str());
        ImGui::TextDisabled("Aktiv: %s", config::profile().c_str());
    }

private:
    static constexpr int slots = 4;
    Setting* keys_[slots]{};
    Setting* names_[slots]{};
};
