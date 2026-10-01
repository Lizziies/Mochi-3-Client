#pragma once

#ifndef MOCHI_VERSION
#define MOCHI_VERSION "0.0.0"
#endif

namespace build {

inline constexpr const char* version = MOCHI_VERSION;
inline constexpr const wchar_t* repoOwner = L"DEIN-GITHUB-NAME";
inline constexpr const wchar_t* repoName = L"mochi";
inline constexpr const wchar_t* repoBranch = L"main";

}
