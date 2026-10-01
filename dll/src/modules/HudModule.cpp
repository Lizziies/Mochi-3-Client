#include "HudModule.hpp"
#include "core/Config.hpp"
#include "gui/Theme.hpp"
#include "render/Fonts.hpp"

#include <algorithm>
#include <cstring>

HudModule::HudModule(std::string name, std::string description, std::vector<std::string> tags, ImVec2 defaultPos)
    : Module(std::move(name), std::move(description), Category::Hud, std::move(tags)),
      background_(toggleSetting("bg", "Background", true)),
      bgColor_(colorSetting("bgColor", "Background color", {0.10f, 0.06f, 0.12f, 0.55f})),
      textColor_(colorSetting("textColor", "Text color", {1.f, 0.95f, 0.97f, 1.f})),
      useAccent_(toggleSetting("accent", "Accent color for labels", true)),
      rounding_(slider("rounding", "Corner radius", 8.f, 0.f, 20.f, "%.0f")),
      padding_(slider("padding", "Padding", 6.f, 0.f, 20.f, "%.0f")),
      shadow_(toggleSetting("shadow", "Text shadow", true)),
      x_(slider("x", "x", defaultPos.x, 0.f, 1.f)),
      y_(slider("y", "y", defaultPos.y, 0.f, 1.f)),
      scale_(slider("scale", "Size", 1.f, 0.4f, 3.f, "%.2fx")) {
    x_.hidden = true;
    y_.hidden = true;
    bgColor_.visible = [this] { return background_.b; };
    rounding_.visible = [this] { return background_.b; };
}

ImVec2 HudModule::position() const {
    auto ds = ImGui::GetIO().DisplaySize;
    return {x_.f * ds.x, y_.f * ds.y - (growsUp() ? lastSize_.y : 0.f)};
}

void HudModule::setPosition(ImVec2 p) {
    auto ds = ImGui::GetIO().DisplaySize;
    if (ds.x <= 0 || ds.y <= 0) return;
    p.x = std::clamp(p.x, 0.f, std::max(0.f, ds.x - lastSize_.x));
    p.y = std::clamp(p.y, 0.f, std::max(0.f, ds.y - lastSize_.y));
    x_.f = p.x / ds.x;
    y_.f = (p.y + (growsUp() ? lastSize_.y : 0.f)) / ds.y;
    config::markDirty();
}

void HudModule::setScale(float s) {
    scale_.f = std::clamp(s, scale_.fmin, scale_.fmax);
    config::markDirty();
}

namespace hud {

static float global = 1.f;

float globalScale() { return global; }

void setGlobalScale(float s) { global = s; }

}

void HudModule::onRender(ImDrawList* dl) {
    float s = scale_.f * hud::globalScale();
    ImVec2 pos = position();
    float pad = padding_.f * s;

    int firstVtx = dl->VtxBuffer.Size;
    dl->ChannelsSplit(2);
    dl->ChannelsSetCurrent(1);
    ImVec2 inner = content(dl, pos + ImVec2(pad, pad), s);
    ImVec2 size = inner + ImVec2(pad * 2, pad * 2);

    dl->ChannelsSetCurrent(0);
    if (background_.b) dl->AddRectFilled(pos, pos + size, ImGui::GetColorU32(bgColor_.color), rounding_.f * s);
    dl->ChannelsMerge();

    if (growsUp() && size.y != lastSize_.y) {
        float dy = lastSize_.y - size.y;
        for (int i = firstVtx; i < dl->VtxBuffer.Size; i++) dl->VtxBuffer[i].pos.y += dy;
    }
    lastSize_ = size;
}

ImVec2 HudModule::textSize(float scale, const std::string& text) const {
    ImFont* f = fonts::hud();
    return f->CalcTextSizeA(fonts::hudSize() * scale, FLT_MAX, 0.f, text.c_str());
}

ImVec2 HudModule::drawText(ImDrawList* dl, ImVec2 at, float scale, const std::string& text, ImU32 color) {
    ImFont* f = fonts::hud();
    float size = fonts::hudSize() * scale;
    if (shadow_.b) dl->AddText(f, size, at + ImVec2(1.f * scale, 1.f * scale), IM_COL32(0, 0, 0, 140), text.c_str());
    dl->AddText(f, size, at, color, text.c_str());
    return f->CalcTextSizeA(size, FLT_MAX, 0.f, text.c_str());
}

ImU32 HudModule::textColor() const { return ImGui::GetColorU32(textColor_.color); }

ImU32 HudModule::accentColor() const {
    return useAccent_.b ? ImGui::GetColorU32(theme::current().accent) : textColor();
}

ImVec2 TextHud::content(ImDrawList* dl, ImVec2 origin, float scale) {
    std::string l = label();
    std::string v = value();
    if (!format_.text.empty()) {
        std::string out = format_.text;
        for (auto& [key, val] : {std::pair<const char*, std::string&>{"{label}", l}, {"{value}", v}})
            for (size_t at = out.find(key); at != std::string::npos; at = out.find(key, at + val.size()))
                out.replace(at, std::strlen(key), val);
        auto sz = drawText(dl, origin, scale, out, valueColor());
        return sz;
    }
    ImVec2 at = origin;
    float h = 0;
    if (!l.empty()) {
        auto sz = drawText(dl, at, scale, l + " ", accentColor());
        at.x += sz.x;
        h = sz.y;
    }
    auto sz = drawText(dl, at, scale, v, valueColor());
    return {at.x + sz.x - origin.x, std::max(h, sz.y)};
}
