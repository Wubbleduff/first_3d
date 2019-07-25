#pragma once

#include "types.h"

#include <windows.h>


void init_renderer(HWND window, u32 framebuffer_width, u32 framebuffer_height, bool is_fullscreen, bool is_vsync);

void render();

void shutdown();



v3 *camera_position();
float *camera_latitude();
float *camera_longitude();

// In degrees
v3 get_camera_to_target(float radius, float latitude, float longitude);

