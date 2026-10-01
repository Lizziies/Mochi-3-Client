#pragma once

#include <string>
#include <vector>

namespace config {

void load();
void save();
void saveIfDirty();
void markDirty();

const std::string& profile();
std::vector<std::string> profiles();
void switchProfile(const std::string& name);
void deleteProfile(const std::string& name);

}
