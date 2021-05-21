#pragma once

#include <windows.h>

#include <vector> // vector for meshes

#include "d3d11.h"
#include "dxgi.h"
#include "d3dcommon.h"
#include "D3DX10math.h"
#include "D3DX11async.h"
#include "D3DX11tex.h"


void init_renderer(HWND window, unsigned framebuffer_width, unsigned framebuffer_height, bool is_fullscreen, bool is_vsync);
void render();
void shutdown_renderer();

