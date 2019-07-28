#pragma once

#include "types.h"

#include <windows.h>

#include <vector> // vector for meshes

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcommon.h>
#include <d3dx10math.h>
#include <d3dx11async.h>
#include <d3dx11tex.h>

struct Mesh
{
  struct Vertex
  {
    v3 position;
    v3 normal;
    v2 uv;

    Vertex(v3 a, v3 b, v2 c) : position(a), normal(b), uv(c) {}
  };

  std::vector<Vertex> vertices;
  //std::vector<v3> vertices;
  //std::vector<v3> normals;
  //std::vector<v3> materials;
  //std::vector<v2> uvs;
  std::vector<u32> indices;


  ID3D11Buffer *vertex_buffer;
  ID3D11Buffer *index_buffer;


  u32 draw_mode = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;


  void normalize();
  void compute_vertex_normals();
  void fill_buffers(ID3D11Device *device);
  void clear_buffers();
};

enum PrimitiveType
{
  PRIMITIVE_QUAD,
};

struct Shader;
struct Texture;
struct Model
{
  v3 position = v3();
  v3 scale = v3(1.0f, 1.0f, 1.0f);
  float y_axis_rotation = 0.0f; // In degrees

  Mesh *mesh = 0;
  Shader *shader = 0;
  Texture *texture = 0;
};

typedef u32 ModelHandle;




void init_renderer(HWND window, u32 framebuffer_width, u32 framebuffer_height, bool is_fullscreen, bool is_vsync);

void render();

void shutdown();


ModelHandle create_model(const s8 *obj_path);
ModelHandle create_model(PrimitiveType primitive, const s8 *texture_path);

Model *get_temp_model_pointer(ModelHandle model);


v3 *camera_position();

v3 *get_camera_looking();

