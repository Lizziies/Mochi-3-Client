#pragma once

#include <imgui.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace capture {

enum class Stage { Game, Overlay };

struct Image {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> bgra;
};

bool request(Stage stage);
bool busy();
bool poll(Image& out);
void submit(ImDrawList* dl, Stage stage);
void shutdown();

enum class Format { Png, Jpeg };

void save(Image image, std::filesystem::path path, Format format, int quality);
bool takeSaved(std::filesystem::path& path, bool& ok);
bool copyToClipboard(const Image& image, void* window);

}
