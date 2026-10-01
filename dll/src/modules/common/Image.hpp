#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace img {

struct Pixels {
    int w = 0;
    int h = 0;
    std::vector<uint32_t> rgba;
};

bool load(const std::filesystem::path& path, int maxSize, Pixels& out);

}
