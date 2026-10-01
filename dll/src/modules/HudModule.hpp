#pragma once

#include "Module.hpp"

#include <string>

class HudModule : public Module {
public:
    HudModule(std::string name, std::string description, std::vector<std::string> tags = {"hud-self"},
              ImVec2 defaultPos = {0.01f, 0.1f});

    bool isHud() const override { return true; }
    void onRender(ImDrawList* dl) override;

    ImVec2 position() const;
    void setPosition(ImVec2 screenPos);
    ImVec2 size() const { return lastSize_; }
    float scale() const { return scale_.f; }
    void setScale(float s);

protected:
    virtual ImVec2 content(ImDrawList* dl, ImVec2 origin, float scale) = 0;

    ImVec2 drawText(ImDrawList* dl, ImVec2 at, float scale, const std::string& text, ImU32 color);
    ImVec2 textSize(float scale, const std::string& text) const;
    ImU32 textColor() const;
    ImU32 accentColor() const;

    Setting& background_;
    Setting& bgColor_;
    Setting& textColor_;
    Setting& useAccent_;
    Setting& rounding_;
    Setting& padding_;
    Setting& shadow_;

private:
    Setting& x_;
    Setting& y_;
    Setting& scale_;
    ImVec2 lastSize_{0, 0};
};

class TextHud : public HudModule {
public:
    using HudModule::HudModule;

protected:
    virtual std::string label() const { return ""; }
    virtual std::string value() = 0;
    ImVec2 content(ImDrawList* dl, ImVec2 origin, float scale) override;
};
