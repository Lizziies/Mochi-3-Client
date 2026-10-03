#pragma once

#include "gui/Notify.hpp"
#include "hook/Input.hpp"
#include "modules/Module.hpp"
#include "modules/common/Needs.hpp"
#include "modules/common/Options.hpp"
#include "modules/common/Text.hpp"
#include "render/Ui.hpp"
#include "sdk/Game.hpp"
#include "sdk/Inject.hpp"

#include <windows.h>

#include <algorithm>
#include <set>

class InventoryLock : public Module {
public:
    InventoryLock()
        : Module("Inventory Lock", "Keeps tools and other valuables from being dropped by accident: dropping needs a quick double press of the drop key.",
                 Category::Comfort, {"input"}) {
        sub("Inventory");
        require(need::inventory, need::sigs({"LocalPlayer"}));
    }

    void onKey(KeyEvent& ev) override {
        if (!ev.down || ev.repeat || !dropKey_.i || ev.vk != dropKey_.i || mcopt::cursorFree() || inject::ours()) return;
        if (!locked(game::state().player.held(), game::state().player.slot)) return;
        double now = ui::time();
        if (now - last_ < gap_.f / 1000.0) {
            last_ = -100.0;
            return;
        }
        last_ = now;
        ev.cancel = true;
        blocked_++;
        if (toast_.b) notify::push(i18n::tr("Inventory Lock"), i18n::fmt("Press {} again to drop it", keyName()), notify::Kind::Info, 1.6f);
    }

    void drawSettings() override {
        ImGui::Spacing();
        ImGui::TextDisabled("%s", i18n::fmt("Drops blocked so far: {}", blocked_).c_str());
    }

private:
    static bool tool(const std::string& n) {
        static const char* parts[] = {"_sword", "_pickaxe", "_axe", "_shovel", "_hoe", "bow", "crossbow", "trident", "shears", "fishing_rod", "flint_and_steel", "mace"};
        for (auto p : parts)
            if (n.find(p) != std::string::npos) return true;
        return false;
    }

    static bool gear(const std::string& n) {
        static const char* parts[] = {"_helmet", "_chestplate", "_leggings", "_boots", "elytra", "totem_of_undying", "shield", "turtle_helmet"};
        for (auto p : parts)
            if (n.find(p) != std::string::npos) return true;
        return false;
    }

    bool locked(const game::Item& it, int slot) const {
        for (auto& s : text::split(slots_.text, ','))
            if (std::atoi(s.c_str()) == slot + 1) return true;
        if (it.empty()) return false;
        if (mode_.i == 1) return true;
        return tool(it.name) || (gear_.b && gear(it.name));
    }

    std::string keyName() const { return dropKey_.i >= 'A' && dropKey_.i <= 'Z' ? std::string(1, char(dropKey_.i)) : i18n::tr("the drop key"); }

    Setting& mode_ = choice("mode", "Protect", {"Tools only", "All items"});
    Setting& gear_ = toggleSetting("gear", "Also armor, shield and totems", true);
    Setting& slots_ = textSetting("slots", "Always protect these hotbar slots (1 to 9, comma)", "");
    Setting& dropKey_ = keySetting("dropKey", "Drop key", 'Q');
    Setting& gap_ = slider("gap", "Time for the second press (ms)", 300.f, 100.f, 800.f, "%.0f ms");
    Setting& toast_ = toggleSetting("toast", "Show a hint when a drop is blocked", true);
    double last_ = -100.0;
    int blocked_ = 0;
};

class ModernKeybinds : public Module {
public:
    ModernKeybinds()
        : Module("Modern Keybind Handling", "Keeps your movement keys going after you close the inventory, chat or a menu while still holding them.", Category::Comfort,
                 {"input"}) {
        sub("Movement");
    }

    void onFrame() override {
        bool free = mcopt::cursorFree();
        double now = ui::time();
        if (wasFree_ && !free) due_ = now + delay_.f / 1000.0;
        wasFree_ = free;
        if (due_ <= 0.0 || now < due_) return;
        due_ = 0.0;
        if (free || !inject::focused()) return;
        for (auto& k : keys()) {
            if (!k.enabled || !(GetAsyncKeyState(k.vk) & 0x8000)) continue;
            inject::key(k.vk, true);
            restored_++;
        }
    }

    void drawSettings() override {
        ImGui::Spacing();
        ImGui::TextDisabled("%s", i18n::fmt("Keys restored so far: {}", restored_).c_str());
    }

private:
    struct Key {
        int vk;
        bool enabled;
    };

    std::array<Key, 7> keys() const {
        return {{{'W', forward_.b}, {'A', left_.b}, {'S', back_.b}, {'D', right_.b}, {VK_SPACE, jump_.b}, {VK_LSHIFT, sneak_.b}, {VK_LCONTROL, sprint_.b}}};
    }

    Setting& forward_ = toggleSetting("forward", "Forward", true);
    Setting& left_ = toggleSetting("left", "Left", true);
    Setting& back_ = toggleSetting("back", "Back", true);
    Setting& right_ = toggleSetting("right", "Right", true);
    Setting& jump_ = toggleSetting("jump", "Jump", true);
    Setting& sneak_ = toggleSetting("sneak", "Sneak", true);
    Setting& sprint_ = toggleSetting("sprint", "Sprint", true);
    Setting& delay_ = slider("delay", "Delay after closing (ms)", 80.f, 20.f, 400.f, "%.0f ms");
    bool wasFree_ = false;
    double due_ = 0.0;
    int restored_ = 0;
};

class JavaInventoryHotkeys : public Module {
public:
    JavaInventoryHotkeys()
        : Module("Java Inventory Hotkeys", "Uses the hotbar keys from your game options in the inventory, like the number keys do.", Category::Comfort, {"input"}) {
        sub("Inventory");
        load();
    }

    void onEnable() override { load(); }

    // the hotbar keys may have been changed in the game's settings, which is a menu like any other
    void onFrame() override {
        bool free = mcopt::cursorFree();
        if (wasFree_ && !free) load();
        wasFree_ = free;
    }

    void onKey(KeyEvent& ev) override {
        if (!ev.down || ev.repeat || inject::ours() || !mcopt::cursorFree()) return;
        for (int i = 0; i < 9; i++) {
            if (!bound_[size_t(i)] || ev.vk != bound_[size_t(i)] || ev.vk == '1' + i) continue;
            ev.cancel = true;
            inject::tapLater('1' + i);
            used_++;
            return;
        }
    }

    void drawSettings() override {
        ImGui::Spacing();
        if (ImGui::SmallButton(i18n::tr("Read the options file again"))) load();
        if (!found_) {
            ImGui::TextDisabled("%s", i18n::tr("The game options file was not found."));
            return;
        }
        std::string line;
        for (int i = 0; i < 9; i++) line += std::format("{}:{} ", i + 1, bound_[size_t(i)] ? name(bound_[size_t(i)]) : "–");
        ImGui::TextDisabled("%s", line.c_str());
        ImGui::TextDisabled("%s", i18n::fmt("Used so far: {}", used_).c_str());
    }

private:
    static std::string name(int vk) {
        if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) return std::string(1, char(vk));
        if (vk >= VK_F1 && vk <= VK_F12) return std::format("F{}", vk - VK_F1 + 1);
        return std::format("#{}", vk);
    }

    void load() {
        bound_.fill(0);
        auto opts = mcopt::readOptions();
        found_ = !opts.empty();
        for (int i = 0; i < 9; i++) {
            auto it = opts.find(std::format("keyboard_type_0_key.hotbar.{}", i + 1));
            if (it != opts.end()) bound_[size_t(i)] = std::atoi(it->second.c_str());
        }
    }

    std::array<int, 9> bound_{};
    bool found_ = false;
    bool wasFree_ = false;
    int used_ = 0;
};
