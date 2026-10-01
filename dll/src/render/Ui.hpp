#pragma once

#include <windows.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace ui {

bool init(HWND window, ID3D11Device* device, ID3D11DeviceContext* context);
void frame();
void shutdown();
void invalidate();

bool wndProc(HWND w, UINT msg, WPARAM wp, LPARAM lp);
bool wantsCursor();

float scale();
float dt();
double time();

}
