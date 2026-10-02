#include "HudModule.hpp"
#include "core/Config.hpp"
#include "gui/Gui.hpp"
#include "gui/Theme.hpp"
#include "modules/common/Context.hpp"
#include "modules/post/PostFx.hpp"
#include "render/Fonts.hpp"

#include <algorithm>
#include <cstring>

HudModule::HudModule(std::string name, std::string description, std::vector<std::string> tags, ImVec2 defaultPos)
    : Module(std::move(name), std::move(description), Category::Hud, std::move(tags)),
      background_(toggleSetting("bg", "Background", true)),
      bgColor_(colorSetting("bgColor", "Background color", {0.08f, 0.08f, 0.09f, 0.55f})),
      textColor_(colorSetting("textColor", "Text color", {0.95f, 0.95f, 0.97f, 1.f})),
      useAccent_(toggleSetting("accent", "Accent color for labels", true)),
      rounding_(slider("rounding", "Corner radius", 8.f, 0.f, 20.f, "%.0f")),
      padding_(slider("padding", "Padding X", 6.f, 0.f, 30.f, "%.0f")),
      shadow_(toggleSetting("shadow", "Text shadow", true)),
      padY_(slider("padY", "Padding Y", 6.f, 0.f, 30.f, "%.0f")),
      shadowOffset_(slider("shadowOffset", "Text shadow offset", 1.f, 0.f, 4.f, "%.1f")),
      align_(choice("align", "Text alignment", {"Left", "Center", "Right"})),
      minWidth_(slider("minWidth", "Minimum width", 0.f, 0.f, 400.f, "%.0f")),
      border_(toggleSetting("border", "Border", false)),
      borderColor_(colorSetting("borderColor", "Border color", {0.23f, 0.65f, 0.93f, 0.9f})),
      borderWidth_(slider("borderWidth", "Border thickness", 1.5f, 0.5f, 6.f, "%.1f")),
      glow_(toggleSetting("glow", "Glow", false)),
      glowColor_(colorSetting("glowColor", "Glow color", {0.23f, 0.65f, 0.93f, 0.8f})),
      glowSize_(slider("glowSize", "Glow size", 10.f, 2.f, 30.f, "%.0f")),
      dropShadow_(toggleSetting("dropShadow", "Box shadow", false)),
      dropShadowColor_(colorSetting("dropShadowColor", "Box shadow color", {0.f, 0.f, 0.f, 0.6f})),
      dropShadowSize_(slider("dropShadowSize", "Box shadow size", 8.f, 2.f, 30.f, "%.0f")),
      blur_(toggleSetting("blur", "Background blur", false)),
      blurRadius_(slider("blurRadius", "Blur strength", 8.f, 2.f, 24.f, "%.0f")),
      rotation_(slider("rotation", "Rotation", 0.f, -180.f, 180.f, "%.0f")),
      placed_(toggleSetting("placed", "placed", false)),
      x_(slider("x", "x", defaultPos.x, 0.f, 1.f)),
      y_(slider("y", "y", defaultPos.y, 0.f, 1.f)),
      scale_(slider("scale", "Size", 1.f, 0.4f, 3.f, "%.2fx")),
      defaultPos_(defaultPos) {
    x_.hidden = true;
    y_.hidden = true;
    placed_.hidden = true;
    for (Setting* st : {&background_, &bgColor_, &textColor_, &useAccent_, &rounding_, &padding_, &shadow_, &padY_, &shadowOffset_, &align_, &minWidth_,
                        &border_, &borderColor_, &borderWidth_, &glow_, &glowColor_, &glowSize_, &dropShadow_, &dropShadowColor_, &dropShadowSize_,
                        &blur_, &blurRadius_, &rotation_, &scale_})
        st->style = true;
    bgColor_.visible = [this] { return background_.b; };
    align_.visible = [this] { return textLayout(); };
    minWidth_.visible = [this] { return textLayout(); };
    rounding_.visible = [this] { return background_.b || border_.b || glow_.b || dropShadow_.b || blur_.b; };
    shadowOffset_.visible = [this] { return shadow_.b; };
    borderColor_.visible = [this] { return border_.b; };
    borderWidth_.visible = [this] { return border_.b; };
    glowColor_.visible = [this] { return glow_.b; };
    glowSize_.visible = [this] { return glow_.b; };
    dropShadowColor_.visible = [this] { return dropShadow_.b; };
    dropShadowSize_.visible = [this] { return dropShadow_.b; };
    blurRadius_.visible = [this] { return blur_.b; };
}

ImVec2 HudModule::position() const {
    auto ds = ImGui::GetIO().DisplaySize;
    ImVec2 pv = pivot();
    ImVec2 p{x_.f * ds.x - pv.x * lastSize_.x, y_.f * ds.y - pv.y * lastSize_.y};
    p.x = std::clamp(p.x, 0.f, std::max(0.f, ds.x - lastSize_.x));
    p.y = std::clamp(p.y, 0.f, std::max(0.f, ds.y - lastSize_.y));
    return p;
}

void HudModule::setPosition(ImVec2 p) {
    auto ds = ImGui::GetIO().DisplaySize;
    if (ds.x <= 0 || ds.y <= 0) return;
    p.x = std::clamp(p.x, 0.f, std::max(0.f, ds.x - lastSize_.x));
    p.y = std::clamp(p.y, 0.f, std::max(0.f, ds.y - lastSize_.y));
    ImVec2 pv = pivot();
    x_.f = (p.x + pv.x * lastSize_.x) / ds.x;
    y_.f = (p.y + pv.y * lastSize_.y) / ds.y;
    placed_.b = true;
    config::markDirty();
}

void HudModule::setScale(float s) {
    scale_.f = std::clamp(s, scale_.fmin, scale_.fmax);
    config::markDirty();
}

static void ringGlow(ImDrawList* dl, ImVec2 min, ImVec2 max, float rounding, ImVec4 c, float spread, ImVec2 offset) {
    const int steps = 8;
    for (int i = steps; i >= 1; i--) {
        float t = float(i) / steps;
        float grow = spread * t;
        ImVec4 cc = c;
        cc.w *= (1.f - t) * (1.f - t) * 0.9f;
        dl->AddRect(min - ImVec2(grow, grow) + offset, max + ImVec2(grow, grow) + offset, ImGui::ColorConvertFloat4ToU32(cc), rounding + grow, 0,
                    spread / steps * 1.6f);
    }
}

namespace {

struct Spot {
    const HudModule* m;
    ImVec2 min, max;
    bool fixed;
};

// rects drawn so far this frame, and all of the previous frame
std::vector<Spot> drawn, previous;
int spotFrame = -1;

bool overlaps(ImVec2 a0, ImVec2 a1, ImVec2 b0, ImVec2 b1) { return a0.x < b1.x && a1.x > b0.x && a0.y < b1.y && a1.y > b0.y; }

}

// A module that sits on its default spot and was never placed keeps clear of what is already on screen: it moves
// below whatever it would cover, and into the next column when it reaches the bottom. Placed modules never move.
void HudModule::makeRoom() {
    if (placed_.b || !autoPlace() || gui::editingHud() || lastSize_.x < 1.f || lastSize_.y < 1.f) return;
    if (x_.f != defaultPos_.x || y_.f != defaultPos_.y) {
        placed_.b = true;
        return;
    }
    std::vector<Spot> others = drawn;
    for (auto& sp : previous)
        if (sp.fixed && sp.m != this) others.push_back(sp);

    auto ds = ImGui::GetIO().DisplaySize;
    float gap = 4.f * hud::globalScale();
    ImVec2 start = position(), p = start;
    float column = 0.f;
    bool moved = false;
    for (int tries = 0; tries < 200; tries++) {
        auto hit = std::find_if(others.begin(), others.end(), [&](const Spot& o) { return o.m != this && overlaps(p, p + lastSize_, o.min, o.max); });
        if (hit == others.end()) break;
        moved = true;
        column = std::max(column, hit->max.x - p.x);
        p.y = hit->max.y + gap;
        if (p.y + lastSize_.y <= ds.y) continue;
        p = {p.x + column + gap, start.y};
        column = 0.f;
        if (p.x + lastSize_.x > ds.x) {
            placed_.b = true;
            return;
        }
    }
    if (moved) {
        setPosition(p);
        return;
    }
    if (++settled_ < 30) return;
    placed_.b = true;
    config::markDirty();
}

namespace hud {

static float global = 1.f;

float globalScale() { return global; }

void setGlobalScale(float s) { global = s; }

}

void HudModule::onRender(ImDrawList* dl) {
    if (ctx::hideModules && !gui::editingHud()) return;
    if (int frame = ImGui::GetFrameCount(); frame != spotFrame) {
        previous.swap(drawn);
        drawn.clear();
        spotFrame = frame;
    }
    makeRoom();
    float s = scale_.f * hud::globalScale();
    ImVec2 pos = position();
    ImVec2 pad{padding_.f * s, padY_.f * s};

    int firstVtx = dl->VtxBuffer.Size;
    dl->ChannelsSplit(2);
    dl->ChannelsSetCurrent(1);
    ImVec2 inner = content(dl, pos + pad, s);
    ImVec2 size = inner + pad * 2;

    dl->ChannelsSetCurrent(0);
    ImVec2 end = pos + size;
    float r = rounding_.f * s;
    if (dropShadow_.b) ringGlow(dl, pos, end, r, dropShadowColor_.color, dropShadowSize_.f * s, {0.f, 3.f * s});
    if (glow_.b) ringGlow(dl, pos, end, r, glowColor_.color, glowSize_.f * s, {0.f, 0.f});
    if (blur_.b && rotation_.f == 0.f) post::blur(dl, pos, end, r, blurRadius_.f * s, {0.f, 0.f, 0.f, 0.f});
    if (background_.b) dl->AddRectFilled(pos, end, ImGui::GetColorU32(bgColor_.color), r);
    if (border_.b) dl->AddRect(pos, end, ImGui::GetColorU32(borderColor_.color), r, 0, borderWidth_.f * s);
    dl->ChannelsMerge();

    ImVec2 pv = pivot();
    ImVec2 shift{(lastSize_.x - size.x) * pv.x, (lastSize_.y - size.y) * pv.y};
    if (shift.x != 0.f || shift.y != 0.f)
        for (int i = firstVtx; i < dl->VtxBuffer.Size; i++) dl->VtxBuffer[i].pos = dl->VtxBuffer[i].pos + shift;
    if (rotation_.f != 0.f) {
        ImVec2 center = pos + shift + size * 0.5f;
        float rad = rotation_.f * 0.0174533f, c = std::cos(rad), sn = std::sin(rad);
        for (int i = firstVtx; i < dl->VtxBuffer.Size; i++) {
            ImVec2 d = dl->VtxBuffer[i].pos - center;
            dl->VtxBuffer[i].pos = center + ImVec2(d.x * c - d.y * sn, d.x * sn + d.y * c);
        }
    }
    lastSize_ = size;
    drawn.push_back({this, pos + shift, pos + shift + size, placed_.b});
}

ImVec2 HudModule::textSize(float scale, const std::string& text) const {
    ImFont* f = fonts::hud();
    return f->CalcTextSizeA(fonts::hudSize() * scale, FLT_MAX, 0.f, text.c_str());
}

ImVec2 HudModule::drawText(ImDrawList* dl, ImVec2 at, float scale, const std::string& text, ImU32 color) {
    ImFont* f = fonts::hud();
    float size = fonts::hudSize() * scale;
    if (shadow_.b) dl->AddText(f, size, at + ImVec2(shadowOffset_.f * scale, shadowOffset_.f * scale), IM_COL32(0, 0, 0, 140), text.c_str());
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
        std::vector<std::pair<std::string, std::string>> map{{"{label}", l}, {"{value}", v}};
        tokens(map);
        std::string out = format_.text;
        for (auto& [key, val] : map)
            for (size_t at = out.find(key); at != std::string::npos; at = out.find(key, at + val.size()))
                out.replace(at, key.size(), val);
        ImVec2 sz = textSize(scale, out);
        float width = std::max(sz.x, minWidth(scale));
        drawText(dl, origin + ImVec2((width - sz.x) * textAlign(), 0.f), scale, out, valueColor());
        return {width, sz.y};
    }
    float lw = l.empty() ? 0.f : textSize(scale, l + " ").x;
    ImVec2 vs = textSize(scale, v);
    float width = std::max(lw + vs.x, minWidth(scale));
    ImVec2 at = origin + ImVec2((width - lw - vs.x) * textAlign(), 0.f);
    float h = vs.y;
    if (!l.empty()) {
        auto sz = drawText(dl, at, scale, l + " ", accentColor());
        at.x += sz.x;
        h = std::max(h, sz.y);
    }
    drawText(dl, at, scale, v, valueColor());
    return {width, h};
}
