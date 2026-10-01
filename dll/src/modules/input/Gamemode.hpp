#pragma once

#include "gui/Notify.hpp"
#include "modules/Module.hpp"
#include "render/Ui.hpp"
#include "sdk/Inject.hpp"

class GamemodeHotkeys : public Module {
public:
    GamemodeHotkeys()
        : Module("Gamemode Hotkeys", "Switch the game mode with a key by typing the command for you. Works where you are allowed to use the command.", Category::Comfort,
                 {"chat"}) {
        sub("Chat");
    }

    void onKey(KeyEvent& ev) override {
        if (!ev.down || ev.repeat || inject::ours()) return;
        struct Mode {
            const char* name;
            int number;
            Setting* key;
        };
        Mode modes[] = {{"survival", 0, &survival_}, {"creative", 1, &creative_}, {"adventure", 2, &adventure_}, {"spectator", 6, &spectator_}};
        for (auto& m : modes) {
            if (!m.key->i || ev.vk != m.key->i) continue;
            double now = ui::time();
            if (now - last_ < 1.5) return;
            last_ = now;
            std::string cmd = format_.text.empty() ? "/gamemode {mode}" : format_.text;
            std::string number = std::to_string(m.number), name = m.name;
            for (auto [key, val] : {std::pair<const char*, std::string>{"{mode}", name}, {"{id}", number}})
                for (size_t at = cmd.find(key); at != std::string::npos; at = cmd.find(key, at + val.size())) cmd.replace(at, std::strlen(key), val);
            inject::say(cmd, chatKey_.i);
            if (toast_.b) notify::push(i18n::tr("Game mode"), cmd, notify::Kind::Info, 2.f);
            return;
        }
    }

private:
    Setting& survival_ = keySetting("survival", "Survival", 0);
    Setting& creative_ = keySetting("creative", "Creative", 0);
    Setting& adventure_ = keySetting("adventure", "Adventure", 0);
    Setting& spectator_ = keySetting("spectator", "Spectator", 0);
    Setting& format_ = textSetting("format", "Command ({mode} or {id})", "/gamemode {mode}");
    Setting& chatKey_ = keySetting("chatKey", "Chat key in game", 'T');
    Setting& toast_ = toggleSetting("toast", "Show the command that is sent", true);
    double last_ = -100.0;
};
