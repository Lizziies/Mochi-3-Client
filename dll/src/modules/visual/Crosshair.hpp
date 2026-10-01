#pragma once

#include "core/Log.hpp"
#include "gui/Gui.hpp"
#include "gui/Theme.hpp"
#include "hook/Input.hpp"
#include "modules/Module.hpp"
#include "modules/common/Colors.hpp"
#include "modules/common/Image.hpp"
#include "render/Draw.hpp"
#include "render/Ui.hpp"
#include "sdk/Effects.hpp"
#include "sdk/Game.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>

class Crosshair : public Module {
public:
    Crosshair()
        : Module("Custom Crosshair",
                 "Your own crosshair with many shapes, a pixel editor and dynamics.",
                 Category::Visual, {"cosmetic"}) {
        sub("Crosshair");
        grid_.hidden = true;
        if (grid_.text.size() != cells * cells) grid_.text = preset(0);

        size_.visible = [this] { return style_.i != 9; };
        gap_.visible = [this] { return style_.i == 0 || style_.i == 3 || style_.i == 5; };
        thickness_.visible = [this] { return style_.i != 9; };
        cell_.visible = [this] { return style_.i == 9; };
        outlineWidth_.visible = [this] { return outline_.b; };
        outlineColor_.visible = [this] { return outline_.b; };
        activeColor_.visible = [this] { return clickColor_.b; };
        moveSpread_.visible = [this] { return dynamic_.b; };
        jumpSpread_.visible = [this] { return dynamic_.b; };
        sneakShrink_.visible = [this] { return dynamic_.b; };
        clickSpread_.visible = [this] { return dynamic_.b; };
        spinSpeed_.visible = [this] { return spin_.b; };
        rainbowSpeed_.visible = [this] { return rainbow_.b; };
        color_.visible = [this] { return !rainbow_.b; };
        targetColor_.visible = [this] { return targetOn_.b; };
        imagePath_.visible = [this] { return style_.i == 10; };
        imageScale_.visible = [this] { return style_.i == 10; };
        imageTint_.visible = [this] { return style_.i == 10; };
        imageTintColor_.visible = [this] { return style_.i == 10 && imageTint_.b; };
        size_.visible = [this] { return style_.i != 9 && style_.i != 10; };
        thickness_.visible = [this] { return style_.i != 9 && style_.i != 10; };
        loadedFor_ = "";
    }

    void onFrame() override {
        if (hideVanilla_.b) fx::skip(fx::Id::HideCrosshair);
    }

    void onRender(ImDrawList* dl) override {
        if (gui::open() || gui::editingHud()) return;
        auto& st = game::state();
        if (game::has(game::Domain::Player)) {
            if (hideThird_.b && st.player.view != game::View::First) return;
            if (hideScreens_.b && st.screen != game::Screen::None) return;
        }
        auto ds = ImGui::GetIO().DisplaySize;
        center_ = {std::floor(ds.x * 0.5f) + 0.5f + offsetX_.f, std::floor(ds.y * 0.5f) + 0.5f + offsetY_.f};

        bool clicking = input::down(VK_LBUTTON);
        pulse_ = draw::approach(pulse_, clicking ? 1.f : 0.f, clicking ? 40.f : 10.f);
        spread_ = draw::approach(spread_, spreadTarget(clicking), 14.f);
        if (spin_.b) angle_ = std::fmod(angle_ + ui::dt() * spinSpeed_.f, 360.f);
        else angle_ = 0.f;
        rad_ = (rotation_.f + angle_) * 0.0174533f;

        ImVec4 col = baseColor();
        bool onPlayer = targetOn_.b && game::has(game::Domain::Target) && st.target.kind == game::Target::Kind::Entity && (!playersOnly_.b || st.target.isPlayer);
        targetMix_ = draw::approach(targetMix_, onPlayer ? 1.f : 0.f, 18.f);
        if (targetMix_ > 0.001f) col = lerp(col, targetColor_.color, targetMix_);
        if (clickColor_.b) col = lerp(col, activeColor_.color, pulse_);
        col.w *= opacity_.f;
        float size = size_.f * (1.f + (clickPulse_.b ? pulse_ * 0.25f : 0.f));

        ImVec4 oc = outlineColor_.color;
        oc.w *= opacity_.f;
        if (outline_.b) shape(dl, size, thickness_.f + outlineWidth_.f * 2.f, ImGui::GetColorU32(oc), true);
        shape(dl, size, thickness_.f, ImGui::GetColorU32(col), false);
    }

    void drawSettings() override {
        ImGui::Spacing();
        if (style_.i == 10) {
            if (ImGui::SmallButton(i18n::tr("Paste path from clipboard"))) {
                if (const char* clip = ImGui::GetClipboardText()) imagePath_.text = clip;
            }
            ImGui::TextDisabled("%s", i18n::tr("Use a PNG with a transparent background, up to 64 pixels."));
            if (!status_.empty()) ImGui::TextColored(theme::current().warn, "%s", status_.c_str());
        }
        ImGui::TextDisabled(i18n::tr("Pixel editor (used for the shape \"Custom grid\")"));
        editor();
    }

private:
    static constexpr int cells = 15;

    static ImVec4 lerp(ImVec4 a, ImVec4 b, float t) {
        return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
    }

    ImVec4 baseColor() const {
        if (!rainbow_.b) return color_.color;
        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(std::fmod(float(ui::time()) * rainbowSpeed_.f * 0.2f, 1.f), 0.55f, 1.f, r, g, b);
        return {r, g, b, color_.color.w};
    }

    float spreadTarget(bool clicking) const {
        if (!dynamic_.b) return 0.f;
        float t = 0.f;
        if (input::down('W') || input::down('A') || input::down('S') || input::down('D')) t += moveSpread_.f;
        if (input::down(VK_SPACE)) t += jumpSpread_.f;
        if (input::down(VK_LSHIFT)) t -= sneakShrink_.f;
        if (clicking) t += clickSpread_.f;
        return t;
    }

    ImVec2 turn(ImVec2 p) const {
        if (rad_ == 0.f) return p;
        float s = std::sin(rad_), c = std::cos(rad_);
        ImVec2 d = p - center_;
        return center_ + ImVec2(d.x * c - d.y * s, d.x * s + d.y * c);
    }

    void line(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 col, float th) const { dl->AddLine(turn(a), turn(b), col, th); }

    void cross(ImDrawList* dl, float size, float th, ImU32 col, bool tShape) const {
        ImVec2 c = center_;
        float g = std::max(0.f, gap_.f + spread_);
        float out = size + spread_;
        line(dl, {c.x - out, c.y}, {c.x - g, c.y}, col, th);
        line(dl, {c.x + g, c.y}, {c.x + out, c.y}, col, th);
        if (!tShape) line(dl, {c.x, c.y - out}, {c.x, c.y - g}, col, th);
        line(dl, {c.x, c.y + g}, {c.x, c.y + out}, col, th);
    }

    void polygon(ImDrawList* dl, int n, float radius, float th, ImU32 col, float start) const {
        ImVec2 pts[8];
        for (int i = 0; i < n; i++) {
            float a = start + 6.2832f * i / n;
            pts[i] = turn(center_ + ImVec2(std::cos(a), std::sin(a)) * radius);
        }
        dl->AddPolyline(pts, n, col, ImDrawFlags_Closed, th);
    }

    void grid(ImDrawList* dl, ImU32 mainCol, ImU32 altCol, bool outline) const {
        float cs = cell_.f * (1.f + spread_ * 0.05f);
        float half = cells * cs * 0.5f;
        float ow = outlineWidth_.f;
        for (int y = 0; y < cells; y++)
            for (int x = 0; x < cells; x++) {
                char v = grid_.text[y * cells + x];
                if (v == '0') continue;
                ImVec2 a = center_ + ImVec2(x * cs - half, y * cs - half);
                ImVec2 b = a + ImVec2(cs, cs);
                if (outline) {
                    dl->AddRectFilled(a - ImVec2(ow, ow), b + ImVec2(ow, ow), mainCol);
                    continue;
                }
                ImU32 c = v == '2' ? altCol : v == '3' ? IM_COL32(0, 0, 0, 230) : mainCol;
                dl->AddRectFilled(a, b, c);
            }
    }

    void shape(ImDrawList* dl, float size, float th, ImU32 col, bool outline) {
        ImVec4 secondary = activeColor_.color;
        secondary.w *= opacity_.f;
        ImU32 alt = ImGui::GetColorU32(secondary);
        switch (style_.i) {
        case 0: cross(dl, size, th, col, false); break;
        case 1: dl->AddCircleFilled(center_, th * 1.2f + (outline ? outlineWidth_.f : 0), col); break;
        case 2: dl->AddCircle(center_, (size + spread_) * 0.6f, col, 32, th); break;
        case 3:
            cross(dl, size, th, col, false);
            dl->AddCircleFilled(center_, th * 0.9f + (outline ? outlineWidth_.f : 0), col);
            break;
        case 4: draw::heart(dl, center_, size * 1.4f + (outline ? outlineWidth_.f * 2.f : 0), col); break;
        case 5: cross(dl, size, th, col, true); break;
        case 6: polygon(dl, 4, (size + spread_) * 0.7f, th, col, 0.7854f); break;
        case 7: polygon(dl, 4, (size + spread_) * 0.8f, th, col, 0.f); break;
        case 8: polygon(dl, 3, (size + spread_) * 0.8f, th, col, -1.5708f); break;
        case 9: grid(dl, col, alt, outline); break;
        case 10: picture(dl, col, outline); break;
        }
        if (centerDot_.b && style_.i != 1 && style_.i != 3 && style_.i != 9 && style_.i != 10)
            dl->AddCircleFilled(center_, th * 0.8f + (outline ? outlineWidth_.f : 0), col);
    }

    void ensureImage() {
        if (imagePath_.text == loadedFor_) return;
        loadedFor_ = imagePath_.text;
        image_ = {};
        if (imagePath_.text.empty()) return;
        std::string clean = imagePath_.text;
        if (clean.size() > 1 && clean.front() == '"' && clean.back() == '"') clean = clean.substr(1, clean.size() - 2);
        if (!img::load(std::filesystem::path(logger::widen(clean)), 64, image_)) status_ = i18n::tr("Could not read the image");
        else status_.clear();
    }

    void picture(ImDrawList* dl, ImU32 col, bool outline) {
        ensureImage();
        if (image_.rgba.empty()) return;
        float px = imageScale_.f * (1.f + spread_ * 0.05f);
        float w = float(image_.w) * px, h = float(image_.h) * px;
        ImVec2 origin = center_ - ImVec2(w, h) * 0.5f;
        ImVec4 tint = imageTintColor_.color;
        float ow = outlineWidth_.f;
        for (int y = 0; y < image_.h; y++)
            for (int x = 0; x < image_.w; x++) {
                uint32_t c = image_.rgba[size_t(y * image_.w + x)];
                int a = int((c >> 24) & 255);
                if (a < 8) continue;
                ImVec2 p0 = origin + ImVec2(float(x) * px, float(y) * px), p1 = p0 + ImVec2(px, px);
                if (outline) {
                    dl->AddRectFilled(p0 - ImVec2(ow, ow), p1 + ImVec2(ow, ow), col);
                    continue;
                }
                ImVec4 v{float(c & 255) / 255.f, float((c >> 8) & 255) / 255.f, float((c >> 16) & 255) / 255.f, float(a) / 255.f * opacity_.f};
                if (imageTint_.b) v = {tint.x * v.x, tint.y * v.y, tint.z * v.z, v.w * tint.w};
                dl->AddRectFilled(p0, p1, ImGui::GetColorU32(v));
            }
    }

    static std::string preset(int kind) {
        std::string g(cells * cells, '0');
        auto set = [&](int x, int y, char v = '1') {
            if (x >= 0 && y >= 0 && x < cells && y < cells) g[y * cells + x] = v;
        };
        int m = cells / 2;
        switch (kind) {
        case 0:
            for (int i = 2; i <= 5; i++) {
                set(m - i, m);
                set(m + i, m);
                set(m, m - i);
                set(m, m + i);
            }
            break;
        case 1:
            set(m, m);
            set(m - 1, m);
            set(m + 1, m);
            set(m, m - 1);
            set(m, m + 1);
            break;
        case 2:
            for (int i = 0; i < 360; i += 15) set(m + int(std::lround(5.f * std::cos(i * 0.0174533f))), m + int(std::lround(5.f * std::sin(i * 0.0174533f))));
            set(m, m, '2');
            break;
        case 3: {
            static const char* heart[7] = {".##.##.", "#######", "#######", ".#####.", "..###..", "...#...", "......."};
            for (int y = 0; y < 7; y++)
                for (int x = 0; x < 7; x++)
                    if (heart[y][x] == '#') set(m - 3 + x, m - 3 + y, y < 2 ? '2' : '1');
            break;
        }
        case 4:
            for (int i = 1; i <= 6; i++) {
                set(m - i, m - i);
                set(m + i, m - i);
                set(m - i, m + i);
                set(m + i, m + i);
            }
            set(m, m, '2');
            break;
        }
        return g;
    }

    std::string exportCode() const {
        static const char* hex = "0123456789ABCDEF";
        std::string out;
        for (size_t i = 0; i < grid_.text.size(); i += 2) {
            int a = grid_.text[i] - '0', b = i + 1 < grid_.text.size() ? grid_.text[i + 1] - '0' : 0;
            out += hex[std::clamp(a, 0, 3) * 4 + std::clamp(b, 0, 3)];
        }
        return "MCH1-" + out;
    }

    bool importCode(const std::string& code) {
        if (!code.starts_with("MCH1-")) return false;
        std::string g;
        for (size_t i = 5; i < code.size(); i++) {
            char c = code[i];
            int v = c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
            if (v < 0) return false;
            g += char('0' + v / 4);
            g += char('0' + v % 4);
        }
        g.resize(cells * cells, '0');
        grid_.text = g;
        return true;
    }

    void shift(int dx, int dy) {
        std::string next(cells * cells, '0');
        for (int y = 0; y < cells; y++)
            for (int x = 0; x < cells; x++) {
                int nx = x + dx, ny = y + dy;
                if (nx >= 0 && ny >= 0 && nx < cells && ny < cells) next[ny * cells + nx] = grid_.text[y * cells + x];
            }
        grid_.text = next;
    }

    void editor() {
        auto& t = theme::current();
        float s = ui::scale();
        float cs = 15.f * s;
        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##grid", {cells * cs, cells * cs});
        bool hovered = ImGui::IsItemHovered();
        auto* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(origin, origin + ImVec2(cells * cs, cells * cs), theme::col(t.surface), 4 * s);

        ImVec2 mouse = ImGui::GetIO().MousePos;
        for (int y = 0; y < cells; y++)
            for (int x = 0; x < cells; x++) {
                ImVec2 a = origin + ImVec2(x * cs, y * cs);
                ImVec2 b = a + ImVec2(cs, cs);
                char v = grid_.text[y * cells + x];
                if (v != '0')
                    dl->AddRectFilled(a + ImVec2(1, 1), b - ImVec2(1, 1),
                                      v == '2' ? ImGui::GetColorU32(activeColor_.color) : v == '3' ? IM_COL32(0, 0, 0, 255) : ImGui::GetColorU32(color_.color));
                dl->AddRect(a, b, theme::col(t.textDim, 0.18f));
                if (hovered && mouse.x >= a.x && mouse.x < b.x && mouse.y >= a.y && mouse.y < b.y) {
                    dl->AddRect(a, b, theme::col(t.accent), 0, 0, 2.f);
                    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) grid_.text[y * cells + x] = char('0' + brush_);
                    if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) grid_.text[y * cells + x] = '0';
                }
            }

        ImGui::TextDisabled(i18n::tr("Left click paints, right click erases"));
        const char* names[] = {"Main color", "Second color", "Black"};
        for (int i = 0; i < 3; i++) {
            if (i) ImGui::SameLine();
            if (ImGui::RadioButton(i18n::tr(names[i]), brush_ == i + 1)) brush_ = i + 1;
        }
        if (ImGui::SmallButton(i18n::tr("Clear"))) grid_.text.assign(cells * cells, '0');
        ImGui::SameLine();
        if (ImGui::SmallButton(i18n::tr("Mirror"))) {
            for (int y = 0; y < cells; y++) std::reverse(grid_.text.begin() + y * cells, grid_.text.begin() + (y + 1) * cells);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(i18n::tr("High"))) shift(0, -1);
        ImGui::SameLine();
        if (ImGui::SmallButton(i18n::tr("Down"))) shift(0, 1);
        ImGui::SameLine();
        if (ImGui::SmallButton(i18n::tr("Left"))) shift(-1, 0);
        ImGui::SameLine();
        if (ImGui::SmallButton(i18n::tr("Right"))) shift(1, 0);

        ImGui::TextDisabled(i18n::tr("Presets"));
        const char* presets[] = {"Cross", "Plus", "Ring", "Heart", "X"};
        for (int i = 0; i < 5; i++) {
            if (i) ImGui::SameLine();
            if (ImGui::SmallButton(i18n::tr(presets[i]))) grid_.text = preset(i);
        }
        if (ImGui::SmallButton(i18n::tr("Copy code"))) ImGui::SetClipboardText(exportCode().c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton(i18n::tr("Paste code"))) {
            const char* clip = ImGui::GetClipboardText();
            if (clip) importCode(clip);
        }
    }

    Setting& style_ = choice("style", "Shape", {"Cross", "Dot", "Circle", "Cross + dot", "Heart", "T shape", "Square", "Diamond", "Triangle", "Custom grid", "PNG image"});
    Setting& size_ = slider("size", "Size", 8.f, 2.f, 40.f, "%.0f");
    Setting& cell_ = slider("cell", "Pixel size", 2.f, 1.f, 6.f, "%.1f");
    Setting& imagePath_ = textSetting("imagePath", "PNG file path", "");
    Setting& imageScale_ = slider("imageScale", "Image pixel size", 1.f, 0.25f, 6.f, "%.2f");
    Setting& imageTint_ = toggleSetting("imageTint", "Tint the image", false);
    Setting& imageTintColor_ = colorSetting("imageTintColor", "Image tint", {1.f, 1.f, 1.f, 1.f});
    Setting& gap_ = slider("gap", "Gap", 2.f, 0.f, 16.f, "%.0f");
    Setting& thickness_ = slider("thickness", "Thickness", 2.f, 1.f, 8.f, "%.1f");
    Setting& opacity_ = slider("opacity", "Opacity", 1.f, 0.1f, 1.f, "%.2f");
    Setting& color_ = colorSetting("color", "Color", {1.f, 1.f, 1.f, 0.95f});
    Setting& rainbow_ = toggleSetting("rainbow", "Rainbow", false);
    Setting& rainbowSpeed_ = slider("rainbowSpeed", "Rainbow speed", 1.f, 0.1f, 5.f, "%.1f");
    Setting& centerDot_ = toggleSetting("centerDot", "Center dot", false);
    Setting& outline_ = toggleSetting("outline", "Outline", true);
    Setting& outlineWidth_ = slider("outlineWidth", "Outline thickness", 1.f, 0.5f, 4.f, "%.1f");
    Setting& outlineColor_ = colorSetting("outlineColor", "Outline color", {0.f, 0.f, 0.f, 0.8f});
    Setting& clickColor_ = toggleSetting("clickColor", "Color while clicking", true);
    Setting& activeColor_ = colorSetting("activeColor", "Click color / second color", {1.f, 0.49f, 0.71f, 1.f});
    Setting& clickPulse_ = toggleSetting("pulse", "Pulse while clicking", true);
    Setting& dynamic_ = toggleSetting("dynamic", "Dynamic (walking, jumping, sneaking)", false);
    Setting& moveSpread_ = slider("moveSpread", "Spread while walking", 3.f, 0.f, 16.f, "%.1f");
    Setting& jumpSpread_ = slider("jumpSpread", "Spread while jumping", 4.f, 0.f, 16.f, "%.1f");
    Setting& sneakShrink_ = slider("sneakShrink", "Shrink while sneaking", 2.f, 0.f, 8.f, "%.1f");
    Setting& clickSpread_ = slider("clickSpread", "Spread while clicking", 2.f, 0.f, 16.f, "%.1f");
    Setting& hideVanilla_ = toggleSetting("hideVanilla", "Hide the original crosshair", true);
    Setting& hideThird_ = toggleSetting("hideThird", "Hide in third person", true);
    Setting& hideScreens_ = toggleSetting("hideScreens", "Hide in inventory, chat and pause", true);
    Setting& targetOn_ = toggleSetting("targetOn", "Color when aiming at an opponent", false);
    Setting& playersOnly_ = toggleSetting("playersOnly", "Only for players", true);
    Setting& targetColor_ = colorSetting("targetColor", "Color when aiming", {1.f, 0.35f, 0.4f, 1.f});
    Setting& rotation_ = slider("rotation", "Rotation", 0.f, 0.f, 360.f, "%.0f°");
    Setting& spin_ = toggleSetting("spin", "Constant spin", false);
    Setting& spinSpeed_ = slider("spinSpeed", "Spin speed (°/s)", 90.f, 10.f, 720.f, "%.0f");
    Setting& offsetX_ = slider("offsetX", "Offset X", 0.f, -100.f, 100.f, "%.0f");
    Setting& offsetY_ = slider("offsetY", "Offset Y", 0.f, -100.f, 100.f, "%.0f");
    Setting& grid_ = textSetting("grid", "Grid", preset(0));

    img::Pixels image_;
    std::string loadedFor_;
    std::string status_;
    ImVec2 center_{0, 0};
    float pulse_ = 0.f;
    float targetMix_ = 0.f;
    float spread_ = 0.f;
    float angle_ = 0.f;
    float rad_ = 0.f;
    int brush_ = 1;
};
