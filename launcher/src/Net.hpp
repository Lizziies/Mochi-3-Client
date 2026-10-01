#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace net {

std::optional<std::string> get(const std::string& url, int timeoutMs = 8000);
bool download(const std::string& url, const std::filesystem::path& to, const std::function<void(float)>& progress);

}
