/* Start Header -------------------------------------------------------
Copyright (C) 2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior written
consent of DigiPen Institute of Technology is prohibited.
File Name: asset_loading.h
Purpose: Interface for the asset loading
Language: C++
Platform: Windows 10 OpenGL 4.0
Project: michael.fritz_CS300_2
Author: Michael Fritz, michael.fritz, 180000117
Creation date: 5/31/2019
End Header --------------------------------------------------------*/

#pragma once

#include "../my_math.h" // vector types

#include <vector>

void load_obj(const char *path_to_obj, std::vector<v3> *vertices,
             std::vector<v2> *texture_coords, std::vector<v3> *normals,
             std::vector<unsigned> *indices);
