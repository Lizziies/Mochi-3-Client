#include "GameText.hpp"
#include "modules/common/Text.hpp"

#include <algorithm>

namespace gameText {

namespace {

ImVec2 layout(ImDrawList* dl, ImFont* font, float height, ImVec2 at, ImU32 color, const std::string& value, float shadow) {
    ImVec2 cursor = at;
    float width = 0.f;
    for (const auto& part : text::colored(value, color)) {
        size_t from = 0;
        while (from < part.text.size()) {
            size_t to = part.text.find('\n', from);
            bool newline = to != std::string::npos;
            if (!newline) to = part.text.size();
            const char* begin = part.text.data() + from;
            const char* end = part.text.data() + to;
            float advance = font->CalcTextSizeA(height, FLT_MAX, 0.f, begin, end).x;
            float weight = part.bold ? height / 16.f : 0.f;
            if (dl && begin != end) {
                auto paint = [&](ImVec2 p, ImU32 ink) {
                    int first = dl->VtxBuffer.Size;
                    dl->AddText(font, height, p, ink, begin, end);
                    if (weight > 0.f) dl->AddText(font, height, p + ImVec2(weight, 0), ink, begin, end);
                    if (part.italic)
                        for (int v = first; v < dl->VtxBuffer.Size; v++)
                            dl->VtxBuffer[v].pos.x += (p.y + height - dl->VtxBuffer[v].pos.y) * 0.2f;
                };
                if (shadow > 0.f) paint(cursor + ImVec2(shadow, shadow), IM_COL32(0, 0, 0, ((part.color >> IM_COL32_A_SHIFT) & 255) * 140 / 255));
                paint(cursor, part.color);
            }
            cursor.x += advance + (advance > 0.f ? weight : 0.f);
            width = std::max(width, cursor.x - at.x);
            if (newline) {
                cursor.x = at.x;
                cursor.y += height;
            }
            from = to + 1;
        }
    }
    return {width, cursor.y - at.y + height};
}

}

ImVec2 size(ImFont* font, float height, const std::string& value) {
    return layout(nullptr, font, height, {}, 0, value, 0.f);
}

ImVec2 draw(ImDrawList* dl, ImFont* font, float height, ImVec2 at, ImU32 color, const std::string& value, float shadow) {
    return layout(dl, font, height, at, color, value, shadow);
}

}
