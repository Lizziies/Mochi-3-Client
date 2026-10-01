#pragma once

#include "modules/Module.hpp"
#include "modules/common/Needs.hpp"
#include "sdk/Effects.hpp"
#include "sdk/Game.hpp"

#include <windows.h>

#include <cmath>
#include <cstring>

class MumbleLink : public Module {
public:
    MumbleLink()
        : Module("Mumble Link", "Sendet deine Position an Mumble, damit die Sprachchat-Lautstärke nach Abstand und Richtung funktioniert.", Category::Comfort,
                 {"hud-self"}) {
        sub("Audio");
        require(need::player, need::sigs({"LocalPlayer"}));
    }

    void onEnable() override {
        map_ = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(Linked), L"MumbleLink");
        if (map_) mem_ = static_cast<Linked*>(MapViewOfFile(map_, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Linked)));
    }

    void onDisable() override {
        if (mem_) {
            std::memset(mem_, 0, sizeof(Linked));
            UnmapViewOfFile(mem_);
        }
        if (map_) CloseHandle(map_);
        mem_ = nullptr;
        map_ = nullptr;
    }

    void onFrame() override {
        if (!mem_) return;
        auto& p = game::state().player;
        float yaw = p.yaw * 0.0174533f, pitch = p.pitch * 0.0174533f;
        float fx = -std::sin(yaw) * std::cos(pitch), fy = -std::sin(pitch), fz = std::cos(yaw) * std::cos(pitch);
        float flip = flipZ_.b ? -1.f : 1.f;
        float pos[3] = {p.pos.x, p.pos.y, p.pos.z * flip}, front[3] = {fx, fy, fz * flip}, top[3] = {0.f, 1.f, 0.f};
        mem_->uiVersion = 2;
        mem_->uiTick++;
        std::memcpy(mem_->avatarPos, pos, sizeof(pos));
        std::memcpy(mem_->avatarFront, front, sizeof(front));
        std::memcpy(mem_->avatarTop, top, sizeof(top));
        float cam[3] = {p.pos.x, p.pos.y + p.eyeHeight, p.pos.z * flip};
        std::memcpy(mem_->camPos, cam, sizeof(cam));
        std::memcpy(mem_->camFront, front, sizeof(front));
        std::memcpy(mem_->camTop, top, sizeof(top));
        wcsncpy(mem_->name, L"Mochi", 255);
        wcsncpy(mem_->description, L"Mochi Client, Minecraft Bedrock", 2047);
        std::wstring who(p.name.begin(), p.name.end());
        wcsncpy(mem_->identity, who.c_str(), 255);
        std::string ctx = "Minecraft:" + (context_.i == 0 ? game::state().server : std::string("global"));
        mem_->contextLen = uint32_t(std::min<size_t>(ctx.size(), 255));
        std::memcpy(mem_->context, ctx.data(), mem_->contextLen);
    }

private:
    struct Linked {
        uint32_t uiVersion;
        uint32_t uiTick;
        float avatarPos[3];
        float avatarFront[3];
        float avatarTop[3];
        wchar_t name[256];
        float camPos[3];
        float camFront[3];
        float camTop[3];
        wchar_t identity[256];
        uint32_t contextLen;
        unsigned char context[256];
        wchar_t description[2048];
    };

    Setting& flipZ_ = toggleSetting("flipZ", "Z-Achse umkehren (Mumble ist linkshändig)", true);
    Setting& context_ = choice("context", "Gruppe", {"Pro Server", "Alle zusammen"});
    Linked* mem_ = nullptr;
    HANDLE map_ = nullptr;
};

class GuiScale : public Module {
public:
    GuiScale()
        : Module("GUI Scale", "Skaliert die Oberfläche des Spiels in feinen Schritten, über die normalen Stufen hinaus.", Category::Comfort, {"cosmetic"}) {
        sub("HUD-Teile");
        require(0, {fx::sig(fx::Id::GuiScale)});
    }

    void onFrame() override { fx::set(fx::Id::GuiScale, scale_.f); }

private:
    Setting& scale_ = slider("scale", "Skalierung", 1.f, 0.5f, 4.f, "%.2fx");
};
