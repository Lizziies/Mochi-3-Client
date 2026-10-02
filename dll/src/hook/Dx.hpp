#pragma once

#include <windows.h>

#include <cstdint>

namespace dx {

enum class Api { None, Dx11, Dx12 };

struct Tuning {
    bool allowTearing = false;
    bool lowLatency = false;
    float fpsLimit = 0.f;
};

struct FrameInfo {
    int64_t presentQpc = 0;
    double frameMs = 0;
    bool tearingSupported = false;
    bool lowLatencyActive = false;
    int bufferCount = 0;
};

bool install();
void unhookTables();
void uninstall();

Api api();
HWND window();
Tuning& tuning();
const FrameInfo& frame();

}
