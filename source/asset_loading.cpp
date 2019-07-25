/* Start Header -------------------------------------------------------
Copyright (C) 2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior written
consent of DigiPen Institute of Technology is prohibited.
File Name: asset_loading.cpp
Purpose: This file reads and loads OBJ files into memory
Language: C++
Platform: Windows 10 OpenGL 4.0
Project: michael.fritz_CS300_2
Author: Michael Fritz, michael.fritz, 180000117
Creation date: 5/31/2019
End Header --------------------------------------------------------*/

#include "asset_loading.h"

#include <stdio.h>
#include <string.h>

static bool is_digit(s8 c)
{
  if(c >= '0' && c <= '9') return true;
  return false;
}

void load_obj(const s8 *path_to_obj, std::vector<v3> *vertices,
             std::vector<v2> *texture_coords, std::vector<v3> *normals,
             std::vector<u32> *indices)
{
  FILE *file = fopen(path_to_obj, "rb");
  if(!file)
  {
    vertices = 0;
    texture_coords = 0;
    normals = 0;
    indices = 0;
    //printf("Could not find obj file %s\n", path_to_obj);
    return;
  }

  u32 file_size;
  fseek(file, 0, SEEK_END);
  file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // + 1 for null termination
  s8 *data = new s8[file_size + 1];
  fread(data, file_size, 1, file);
  data[file_size] = 0;

  const s8 * delims = " \t\n\r";

  // Tokenize the file
  s8 *tokens = strtok(data, delims);

  u32 lines = 0;
  while(tokens != 0)
  {
    lines++;
    bool read_next_token = true;
    if(strcmp(tokens, "v") == 0)
    {
      // Vertices have format v 0.0 1.0 2.0
      v3 vertex;

      tokens = strtok(NULL, delims);
      vertex.x = (f32)atof(tokens);
      tokens = strtok(NULL, delims);
      vertex.y = (f32)atof(tokens);
      tokens = strtok(NULL, delims);
      vertex.z = (f32)atof(tokens);

      vertices->push_back(vertex);
    }
    else if(strcmp(tokens, "f") == 0)
    {
      // Faces have format f 1 2 3 ...
      u32 index0;
      u32 index1;
      u32 index2;

      tokens = strtok(NULL, delims);
      index0 = atoi(tokens); 
      tokens = strtok(NULL, delims);
      index1 = atoi(tokens);
      tokens = strtok(NULL, delims);
      index2 = atoi(tokens);

      indices->push_back(index0 - 1); // - 1 for OBJ files reading indices starting at 1 (not 0)
      indices->push_back(index1 - 1);
      indices->push_back(index2 - 1);

      read_next_token = false;
      tokens = strtok(NULL, delims);

      while(tokens != 0 && is_digit(tokens[0]))
      {
        /*
        index0 = index1;
        index1 = index2;
        index2 = atoi(tokens);
        */

        index0 = index0;
        index1 = index2;
        index2 = atoi(tokens);;

        indices->push_back(index0 - 1);
        indices->push_back(index1 - 1);
        indices->push_back(index2 - 1);
        
        tokens = strtok(NULL, delims);
      }
    }

    if(read_next_token)
    {
      tokens = strtok(NULL, delims);
    }
  }

  delete [] data;
}

