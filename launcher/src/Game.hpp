#pragma once

#include <windows.h>

#include <atomic>
#include <filesystem>
#include <optional>
#include <string>

namespace game {

std::optional<DWORD> running();
std::string installedVersion();
bool launch();
bool waitReady(DWORD pid, int timeoutMs);
bool injected(DWORD pid);
bool inject(DWORD pid, const std::filesystem::path& dll, std::string& error);

}
