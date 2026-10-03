#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace update {

struct Release {
    std::string tag;
    std::string notes;
    std::string dllUrl;
    std::string coreUrl;
    std::string launcherUrl;
    std::string sumsUrl;
};

std::optional<Release> latest(bool beta);
bool newer(const std::string& candidate, const std::string& current);

std::string installedTag();
bool installDll(const Release& r, const std::function<void(float)>& progress, std::string& error);
bool swapLauncher(const Release& r, const std::function<void(float)>& progress, std::string& error);
void cleanup();

}
