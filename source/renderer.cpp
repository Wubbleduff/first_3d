#include "my_math.h" // v2
#include "renderer.h"
#include "asset_loading.h" // Loading models


#include <d3d11.h>
#include <dxgi.h>
#include <d3dcommon.h>
#include <d3dx10math.h>
#include <d3dx11async.h>
#include <d3dx11tex.h>

#include <assert.h>

#include <vector> // vector for meshes


// Renderer target info
struct Window
{
  u32 framebuffer_width;
  u32 framebuffer_height;
  f32 aspect_ratio;
  bool vsync;
  bool fullscreen;

  f32 background_color[4];
};

// DirectX state
struct D3DResources
{
  u32 video_card_memory_bytes;
  u8 video_card_description[128];
  IDXGISwapChain *swap_chain;
  ID3D11Device *device;
  ID3D11DeviceContext *device_context;
  ID3D11RenderTargetView *render_target_view;
  ID3D11Texture2D *depth_stencil_buffer;
  ID3D11DepthStencilState *depth_stencil_state;
  ID3D11DepthStencilView *depth_stencil_view;
  ID3D11RasterizerState *raster_state;
};

// Shaders
struct Shader
{
  ID3D11VertexShader *vertex_shader;
  ID3D11PixelShader *pixel_shader;
  ID3D11InputLayout *layout;
  ID3D11Buffer *matrix_buffer;
};

// Textures
struct Texture
{
  ID3D11ShaderResourceView *texture_resource;
  ID3D11SamplerState *sample_state;
};



struct MatrixBufferType
{
  mat4 world_m_model;
  mat4 view_m_world;
  mat4 clip_m_view;
};

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


  void normalize()
  {
    // Get the min and max values for each axis
    f32 min_x = INFINITY;
    f32 max_x = -INFINITY;
    f32 min_y = INFINITY;
    f32 max_y = -INFINITY;
    f32 min_z = INFINITY;
    f32 max_z = -INFINITY;
    v3 sum_points = {0.0f, 0.0f, 0.0f};
    for(u32 i = 0; i < vertices.size(); i++)
    {
      min_x = min(min_x, vertices[i].position.x);
      max_x = max(max_x, vertices[i].position.x);
      min_y = min(min_y, vertices[i].position.y);
      max_y = max(max_y, vertices[i].position.y);
      min_z = min(min_z, vertices[i].position.z);
      max_z = max(max_z, vertices[i].position.z);

      sum_points += vertices[i].position;
    }

    // Find center of model
    sum_points /= (f32)vertices.size();

    // Get the max difference in an axis
    f32 diff_x = max_x - min_x;
    f32 diff_y = max_y - min_y;
    f32 diff_z = max_z - min_z;

    f32 max_diff = max(diff_x, max(diff_x, diff_y));

    // Move vertices to center and scale down
    for(u32 i = 0; i < vertices.size(); i++)
    {
      // Move centroid to origin
      vertices[i].position -= sum_points;

      // Scale down to between -1 and 1
      vertices[i].position = (vertices[i].position / max_diff) * 2.0f;
    }
  }

  void compute_vertex_normals()
  {
    if(vertices.size() == 0) return;

    // Allocate buffers
    //normals.reserve(vertices.size());
    //normals.resize(vertices.size());

    std::vector<f32> sums;
    sums.reserve(vertices.size());
    sums.resize(vertices.size());

    // Initalize to zero
    for(u32 i = 0; i < vertices.size(); i++)
    {
      vertices[i].normal = v3(0.0f, 0.0f, 0.0f);
    }
    for(u32 i = 0; i < sums.size(); i++)
    {
      sums[i] = 0.0f;
    }

    // Find the sum of all normals per vertex
    for(u32 i = 0; i < indices.size(); )
    {
      u32 p0_index = indices[i++];
      u32 p1_index = indices[i++];
      u32 p2_index = indices[i++];

      v3 p0 = vertices[p0_index].position;
      v3 p1 = vertices[p1_index].position;
      v3 p2 = vertices[p2_index].position;

      v3 normal = cross(p1 - p0, p2 - p0);
      if(length_squared(normal) == 0.0f)
      {
        normal = v3(0.0f, 0.0f, 0.0f);
      }
      else
      {
        normal = unit(normal);
      }


      vertices[p0_index].normal += normal;
      vertices[p1_index].normal += normal;
      vertices[p2_index].normal += normal;

      sums[p0_index] += 1.0f;
      sums[p1_index] += 1.0f;
      sums[p2_index] += 1.0f;
    }

    // Divide to find average normal per vertex
    for(u32 i = 0; i < vertices.size(); i++)
    {
      if(sums[i] == 0.0f) continue;
      vertices[i].normal /= sums[i];
    }
  }

  void fill_buffers(ID3D11Device *device)
  {
    // Vertices
    {
      // Set up the description of the static vertex buffer.
      D3D11_BUFFER_DESC vertex_buffer_desc;
      vertex_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
      vertex_buffer_desc.ByteWidth = sizeof(Vertex) * vertices.size();
      vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
      vertex_buffer_desc.CPUAccessFlags = 0;
      vertex_buffer_desc.MiscFlags = 0;
      vertex_buffer_desc.StructureByteStride = 0;

      // Give the subresource structure a pointer to the vertex data.
      D3D11_SUBRESOURCE_DATA vertex_data;
      vertex_data.pSysMem = vertices.data();
      vertex_data.SysMemPitch = 0;
      vertex_data.SysMemSlicePitch = 0;

      // Now create the vertex buffer.
      HRESULT result = device->CreateBuffer(&vertex_buffer_desc, &vertex_data, &vertex_buffer);
      assert(!FAILED(result));
    }

    // Indices
    {
      D3D11_BUFFER_DESC index_buffer_desc;
      index_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
      index_buffer_desc.ByteWidth = sizeof(u32) * indices.size();
      index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
      index_buffer_desc.CPUAccessFlags = 0;
      index_buffer_desc.MiscFlags = 0;
      index_buffer_desc.StructureByteStride = 0;

      D3D11_SUBRESOURCE_DATA index_data;
      index_data.pSysMem = indices.data();
      index_data.SysMemPitch = 0;
      index_data.SysMemSlicePitch = 0;

      HRESULT result = device->CreateBuffer(&index_buffer_desc, &index_data, &index_buffer);
      assert(!FAILED(result));
    }
  }

  void clear_buffers()
  {
    if(index_buffer) index_buffer->Release();
    if(vertex_buffer) vertex_buffer->Release();
  }
};

struct Model
{
  v3 position;
  v3 scale;

  // In degrees
  float y_axis_rotation;

  Mesh *mesh;
};

struct Camera
{
  v3 position = v3(0.0f, 1.0f, 5.0f);

  // In degrees
  float latitude = 90.0f;
  float longitude = -90.0f;
  float field_of_view = deg_to_rad(60.0f);
};

struct RendererData
{
  Window window;
  D3DResources resources;

  Shader first_shader;

  Texture wizard_hat;

  std::vector<Model> models_to_render;
  
  Camera camera;
};

// TODO: This is global. Move it somewhere nice.
static RendererData *renderer_data;

static const v3 WORLD_UP_VECTOR = {0.0f, 1.0f, 0.0f};



static void set_shader_paramters(ID3D11DeviceContext *device_context, ID3D11Buffer *matrix_buffer,
                                 const mat4 *world_m_model, const mat4 *view_m_world, const mat4 *clip_m_view,
                                 ID3D11ShaderResourceView *texture
                                )
{
  HRESULT result;
  D3D11_MAPPED_SUBRESOURCE mapped_resource;
  MatrixBufferType *data;
  u32 buffer_number;

  // Lock the constant buffer so it can be written to.
  result = device_context->Map(matrix_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
  assert(!FAILED(result));

  // Get a pointer to the data in the constant buffer.
  data = (MatrixBufferType *)mapped_resource.pData;

  // Copy the matrices into the constant buffer.
  data->world_m_model = *world_m_model;
  data->view_m_world = *view_m_world;
  data->clip_m_view = *clip_m_view;

  // Unlock the constant buffer.
  device_context->Unmap(matrix_buffer, 0);

  // Set the position of the constant buffer in the vertex shader.
  buffer_number = 0;

  // Finanly set the constant buffer in the vertex shader with the updated values.
  device_context->VSSetConstantBuffers(buffer_number, 1, &matrix_buffer);

  // Set shader texture resource in the pixel shader.
  device_context->PSSetShaderResources(0, 1, &texture);
}

static void outputShaderErrorMessage(ID3D10Blob *error_message, HWND hwnd, const s8 *shader_file)
{
  s8 *compile_errors;
  u32 buffer_size;

  // Get a pointer to the error message text buffer.
  compile_errors = (s8 *)(error_message->GetBufferPointer());

  // Get the length of the message.
  buffer_size = error_message->GetBufferSize();

  // Write out the error message.
  compile_errors[buffer_size - 1] = '\0';
  // fprintf_s(logFile, compile_errors);
  // fprintf_s(logFile, "\n");

  // Close the file
  // fclose(logFile);

  // Stop program when there's an error
  assert(0);

  // Release the error message.
  error_message->Release();
  error_message = 0;



  // Pop a message up on the screen to notify the user to check the text file for compile errors.
  //MessageBox(hwnd, "Error compiling shader.  Check shader-error.txt for message.", shader_file, MB_OK);
}

void init_renderer(HWND window, u32 in_framebuffer_width, u32 in_framebuffer_height, bool is_fullscreen, bool is_vsync)
{
  renderer_data = new RendererData();

  renderer_data->window.fullscreen = is_fullscreen;
  renderer_data->window.vsync = is_vsync;
  renderer_data->window.framebuffer_width = in_framebuffer_width;
  renderer_data->window.framebuffer_height = in_framebuffer_height;

  HRESULT result;

  // All references to the renderer data's memory for easier coding :)))
  u32 &video_card_memory_bytes = renderer_data->resources.video_card_memory_bytes;
  u32 &framebuffer_width = renderer_data->window.framebuffer_width;
  u32 &framebuffer_height = renderer_data->window.framebuffer_height;
  bool &vsync = renderer_data->window.vsync;
  bool &fullscreen = renderer_data->window.fullscreen;
  f32 &aspect_ratio = renderer_data->window.aspect_ratio;

  IDXGISwapChain *&swap_chain = renderer_data->resources.swap_chain;
  ID3D11Device *&device = renderer_data->resources.device;
  ID3D11DeviceContext *&device_context = renderer_data->resources.device_context;
  ID3D11RenderTargetView *&render_target_view = renderer_data->resources.render_target_view;
  ID3D11Texture2D *&depth_stencil_buffer = renderer_data->resources.depth_stencil_buffer;
  ID3D11DepthStencilState *&depth_stencil_state = renderer_data->resources.depth_stencil_state;
  ID3D11DepthStencilView *&depth_stencil_view = renderer_data->resources.depth_stencil_view;
  ID3D11RasterizerState *&raster_state = renderer_data->resources.raster_state;


  // Create a DirectX graphics interface factory.
  IDXGIFactory *factory;
  result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&factory);
  assert(!FAILED(result));


  // Use the factory to create an adapter for the primary graphics interface (video card).
  IDXGIAdapter *adapter;
  result = factory->EnumAdapters(0, &adapter);
  assert(!FAILED(result));

  // Enumerate the primary adapter output (monitor).
  IDXGIOutput *adapter_output;
  result = adapter->EnumOutputs(0, &adapter_output);
  assert(!FAILED(result));

  // Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
  u32 num_modes;
  result = adapter_output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &num_modes, NULL);
  assert(!FAILED(result));

  // Create a list to hold all the possible display modes for this monitor/video card combination.
  DXGI_MODE_DESC *display_mode_list= new DXGI_MODE_DESC[num_modes];

  // Now fill the display mode list structures.
  result = adapter_output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &num_modes, display_mode_list);
  assert(!FAILED(result));

  // Get the adapter (video card) description.
  DXGI_ADAPTER_DESC adapter_desc;
  result = adapter->GetDesc(&adapter_desc);
  assert(!FAILED(result));

  // Store the dedicated video card memory in megabytes.
  video_card_memory_bytes = (u32)adapter_desc.DedicatedVideoMemory;

  // Convert the name of the video card to a character array and store it.
  // TODO:
  // u32 string_length;
  // error = wcstombs_s(&string_length, video_card_description, 128, adapter_desc.Description, 128);
  // assert(error == 0);

  // Release the display mode list.
  delete [] display_mode_list;
  display_mode_list = 0;

  // Release the adapter output.
  adapter_output->Release();
  adapter_output = 0;

  // Release the adapter.
  adapter->Release();
  adapter = 0;

  // Release the factory.
  factory->Release();
  factory = 0;



  // Set to a single back buffer.
  DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
  swap_chain_desc.BufferCount = 1;

  // Set the width and height of the back buffer.
  swap_chain_desc.BufferDesc.Width = framebuffer_width;
  swap_chain_desc.BufferDesc.Height = framebuffer_height;

  // Set regular 32-bit surface for the back buffer.
  swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

  // Set the refresh rate of the back buffer.
  if(vsync)
  {
    // VSYNC NOT SUPPORTED YET
    assert(0);

#if 0
    // Now go through all the display modes and find the one that matches the screen width and height.
    // When a match is found store the numerator and denominator of the refresh rate for that monitor.

    // THIS IS NOT CORRECT
    // There are multiple refresh rates for the same dimensions of the monitor and this will just pick the first refresh rate in that list.
    // There needs to be some better way to get the current refresh rate of the monitor.
    u32 numerator;
    u32 denominator;
    for(u32 i = 0; i < num_modes; i++)
    {
      if(display_mode_list[i].Width == (u32)framebuffer_width)
      {
        if(display_mode_list[i].Height == (u32)framebuffer_height)
        {
          numerator = display_mode_list[i].RefreshRate.Numerator;
          denominator = display_mode_list[i].RefreshRate.Denominator;
        }
      }
    }

    swap_chain_desc.BufferDesc.RefreshRate.Numerator = numerator;
    swap_chain_desc.BufferDesc.RefreshRate.Denominator = denominator;
#endif
  }
  else
  {
    swap_chain_desc.BufferDesc.RefreshRate.Numerator = 0;
    swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
  }

  // Set the usage of the back buffer.
  swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

  // Set the handle for the window to render to.
  swap_chain_desc.OutputWindow = window;

  // Turn multisampling off.
  swap_chain_desc.SampleDesc.Count = 1;
  swap_chain_desc.SampleDesc.Quality = 0;

  // Set to full screen or windowed mode.
  if(fullscreen == true)
  {
    swap_chain_desc.Windowed = false;
  }
  else
  {
    swap_chain_desc.Windowed = true;
  }

  // Set the scan line ordering and scaling to unspecified.
  swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

  // Discard the back buffer contents after presenting.
  swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  // Advanced flags.
  swap_chain_desc.Flags = 0; // DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  // Set the feature level to DirectX 11.
  D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
  // Create the swap chain, Direct3D device, and Direct3D device context.
  result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &feature_level, 1, 
                                         D3D11_SDK_VERSION, &swap_chain_desc, &swap_chain, &device, NULL, &device_context
                                        );
  assert(!FAILED(result));

  // Get the pointer to the back buffer.
  ID3D11Texture2D *back_buffer;
  result = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *)&back_buffer);
  assert(!FAILED(result));

  // Create the render target view with the back buffer pointer.
  result = device->CreateRenderTargetView(back_buffer, NULL, &render_target_view);
  assert(!FAILED(result));

  // Release pointer to the back buffer as we no longer need it.
  back_buffer->Release();
  back_buffer = 0;

  // Set up the description of the depth buffer.
  D3D11_TEXTURE2D_DESC depth_buffer_desc = {};
  depth_buffer_desc.Width = framebuffer_width;
  depth_buffer_desc.Height = framebuffer_height;
  depth_buffer_desc.MipLevels = 1;
  depth_buffer_desc.ArraySize = 1;
  depth_buffer_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  depth_buffer_desc.SampleDesc.Count = 1;
  depth_buffer_desc.SampleDesc.Quality = 0;
  depth_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
  depth_buffer_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
  depth_buffer_desc.CPUAccessFlags = 0;
  depth_buffer_desc.MiscFlags = 0;

  // Create the texture for the depth buffer using the filled out description.
  result = device->CreateTexture2D(&depth_buffer_desc, NULL, &depth_stencil_buffer);
  assert(!FAILED(result));

  // Set up the description of the stencil state.
  D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {};
  depth_stencil_desc.DepthEnable = true;
  depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
  depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;

  depth_stencil_desc.StencilEnable = true;
  depth_stencil_desc.StencilReadMask = 0xFF;
  depth_stencil_desc.StencilWriteMask = 0xFF;

  // Stencil operations if pixel is front-facing.
  depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
  depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

  // Stencil operations if pixel is back-facing.
  depth_stencil_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  depth_stencil_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
  depth_stencil_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  depth_stencil_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

  // Create the depth stencil state.
  result = device->CreateDepthStencilState(&depth_stencil_desc, &depth_stencil_state);
  assert(!FAILED(result));

  // Set the depth stencil state.
  device_context->OMSetDepthStencilState(depth_stencil_state, 1);

  // Set up the depth stencil view description.
  D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = {};
  depth_stencil_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
  depth_stencil_view_desc.Texture2D.MipSlice = 0;

  // Create the depth stencil view.
  result = device->CreateDepthStencilView(depth_stencil_buffer, &depth_stencil_view_desc, &depth_stencil_view);
  assert(!FAILED(result));

  // Bind the render target view and depth stencil buffer to the output render pipeline.
  device_context->OMSetRenderTargets(1, &render_target_view, depth_stencil_view);

  // Setup the raster description which will determine how and what polygons will be drawn.
  D3D11_RASTERIZER_DESC raster_desc = {};
  raster_desc.AntialiasedLineEnable = false;
  raster_desc.CullMode = D3D11_CULL_BACK;
  raster_desc.DepthBias = 0;
  raster_desc.DepthBiasClamp = 0.0f;
  raster_desc.DepthClipEnable = true;
  raster_desc.FillMode = D3D11_FILL_SOLID;
  raster_desc.FrontCounterClockwise = true;
  raster_desc.MultisampleEnable = false;
  raster_desc.ScissorEnable = false;
  raster_desc.SlopeScaledDepthBias = 0.0f;

  // Create the rasterizer state from the description we just filled out.
  result = device->CreateRasterizerState(&raster_desc, &raster_state);
  assert(!FAILED(result));

  // Now set the rasterizer state.
  device_context->RSSetState(raster_state);

  // Setup the viewport for rendering.
  D3D11_VIEWPORT viewport = {};
  viewport.Width = (f32)framebuffer_width;
  viewport.Height = (f32)framebuffer_height;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
  viewport.TopLeftX = 0.0f;
  viewport.TopLeftY = 0.0f;

  // Create the viewport.
  device_context->RSSetViewports(1, &viewport);

  aspect_ratio = (f32)framebuffer_width / (f32)framebuffer_height;







  // Shaders

  {
    ID3D10Blob *error_message;
    ID3D10Blob *vertex_shader_buffer;
    ID3D10Blob *pixel_shader_buffer;
    D3D11_INPUT_ELEMENT_DESC polygon_layout[3];
    u32 num_elements;
    D3D11_BUFFER_DESC matrix_buffer_desc;

    error_message = 0;
    vertex_shader_buffer = 0;
    pixel_shader_buffer = 0;

    char *vsFileName = "Shaders/VertexShader.vs";
    char *fsFileName = "Shaders/FragShader.fs";

    // Compile the vertex shader code.
    result = D3DX11CompileFromFile(vsFileName, NULL, NULL, "ColorVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL, 
                                   &vertex_shader_buffer, &error_message, NULL
                                  );
    if(FAILED(result))
    {
      // If the shader failed to compile it should have writen something to the error message.
      if(error_message)
      {
        outputShaderErrorMessage(error_message, window, vsFileName);
      }
      // If there was nothing in the error message then it simply could not find the shader file itself.
      else
      {
        MessageBox(window, vsFileName, "Missing Shader File", MB_OK);
      }

      return;
    }

    // Compile the pixel shader code.
    result = D3DX11CompileFromFile(fsFileName, NULL, NULL, "ColorPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL, 
                                   &pixel_shader_buffer, &error_message, NULL
                                  );
    if(FAILED(result))
    {
      // If the shader failed to compile it should have writen something to the error message.
      if(error_message)
      {
        outputShaderErrorMessage(error_message, window, fsFileName);
      }
      // If there was  nothing in the error message then it simply could not find the file itself.
      else
      {
        MessageBox(window, fsFileName, "Missing Shader File", MB_OK);
      }

      return;
    }

    // Create the vertex shader from the buffer.
    result = device->CreateVertexShader(vertex_shader_buffer->GetBufferPointer(), vertex_shader_buffer->GetBufferSize(), NULL, &renderer_data->first_shader.vertex_shader);
    assert(!FAILED(result));

    // Create the pixel shader from the buffer.
    result = device->CreatePixelShader(pixel_shader_buffer->GetBufferPointer(), pixel_shader_buffer->GetBufferSize(), NULL, &renderer_data->first_shader.pixel_shader);
    assert(!FAILED(result));

    // Now setup the layout of the data that goes into the shader.
    // This needs to match the layout of the structures inside the shader
    polygon_layout[0].SemanticName = "POSITION";
    polygon_layout[0].SemanticIndex = 0;
    polygon_layout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygon_layout[0].InputSlot = 0;
    polygon_layout[0].AlignedByteOffset = 0;
    polygon_layout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygon_layout[0].InstanceDataStepRate = 0;

#if 1
    polygon_layout[1].SemanticName = "NORMAL";
    polygon_layout[1].SemanticIndex = 0;
    polygon_layout[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygon_layout[1].InputSlot = 0;
    polygon_layout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygon_layout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygon_layout[1].InstanceDataStepRate = 0;
#endif

    polygon_layout[2].SemanticName = "TEXCOORD";
    polygon_layout[2].SemanticIndex = 0;
    polygon_layout[2].Format = DXGI_FORMAT_R32G32_FLOAT;
    polygon_layout[2].InputSlot = 0;
    polygon_layout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygon_layout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygon_layout[2].InstanceDataStepRate = 0;

    // Get a count of the elements in the layout.
    num_elements = sizeof(polygon_layout) / sizeof(polygon_layout[0]);

    // Create the vertex input layout.
    result = device->CreateInputLayout(polygon_layout, num_elements, vertex_shader_buffer->GetBufferPointer(), 
                                       vertex_shader_buffer->GetBufferSize(), &renderer_data->first_shader.layout
                                      );
    assert(!FAILED(result));

    // Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
    vertex_shader_buffer->Release();
    vertex_shader_buffer = 0;

    pixel_shader_buffer->Release();
    pixel_shader_buffer = 0;

    // Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
    matrix_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    matrix_buffer_desc.ByteWidth = sizeof(MatrixBufferType);
    matrix_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    matrix_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    matrix_buffer_desc.MiscFlags = 0;
    matrix_buffer_desc.StructureByteStride = 0;

    // Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
    result = device->CreateBuffer(&matrix_buffer_desc, NULL, &renderer_data->first_shader.matrix_buffer);
    assert(!FAILED(result));
  }












  ID3D11BlendState *blendState = NULL;
  D3D11_BLEND_DESC blendStateDesc = {};
  
  blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
  blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
  blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
  blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
  blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
  blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
  blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
  blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0f;

  device->CreateBlendState(&blendStateDesc, &blendState);

  f32 blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  device_context->OMSetBlendState(blendState, blendFactor, 0xffffffff);



  // Textures
  D3D11_SAMPLER_DESC samplerDesc;
  
  result = D3DX11CreateShaderResourceViewFromFile(device, "assets/WizardHat.png", NULL, NULL, &renderer_data->wizard_hat.texture_resource, NULL);
  //result = D3DX11CreateShaderResourceViewFromFile(device, "assets/Vivi.png", NULL, NULL, &textures[1], NULL);
  assert(!FAILED(result));

  // Create a texture sampler state description.
  samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
  samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
  samplerDesc.MipLODBias = 0.0f;
  samplerDesc.MaxAnisotropy = 1;
  samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
  samplerDesc.BorderColor[0] = 0;
  samplerDesc.BorderColor[1] = 0;
  samplerDesc.BorderColor[2] = 0;
  samplerDesc.BorderColor[3] = 0;
  samplerDesc.MinLOD = 0;
  samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

  // Create the texture sampler state.
  result = device->CreateSamplerState(&samplerDesc, &renderer_data->wizard_hat.sample_state);
  assert(!FAILED(result));





  Mesh *teapot = new Mesh();
  std::vector<v3> vertices;
  load_obj("assets/teapot.obj", &vertices, 0, 0, &teapot->indices);
  for(v3 vertex : vertices)
  {
    teapot->vertices.push_back(Mesh::Vertex(vertex, v3(), v2()));
  }
  teapot->normalize();
  teapot->compute_vertex_normals();
  teapot->fill_buffers(device);

  Model model;
  model.position = v3(0.0f, 1.0f, 0.0f);
  model.scale = v3(1.0f, 1.0f, 1.0f);
  model.y_axis_rotation = -45.0f;
  model.mesh = teapot;

  renderer_data->models_to_render.push_back(model);



  vertices.clear();

  Mesh *hat = new Mesh();
  load_obj("assets/wizard_hat2.obj", &vertices, 0, 0, &hat->indices);
  for(v3 vertex : vertices)
  {
    hat->vertices.push_back(Mesh::Vertex(vertex, v3(), v2()));
  }
  hat->normalize();
  hat->compute_vertex_normals();
  hat->fill_buffers(device);

  model.position = v3(0.0f, 1.65f, 0.0f);
  model.scale = v3(0.8f, 0.8f, 0.8f);
  model.y_axis_rotation = 45.0f;
  model.mesh = hat;

  renderer_data->models_to_render.push_back(model);



  vertices.clear();

  Mesh *ground = new Mesh();
  ground->vertices.push_back(Mesh::Vertex(v3(-1.0f, 0.0f,  1.0f), v3(0.0f, 1.0f, 0.0f), v2(0.0f, 1.0f)));
  ground->vertices.push_back(Mesh::Vertex(v3( 1.0f, 0.0f,  1.0f), v3(0.0f, 1.0f, 0.0f), v2(1.0f, 1.0f)));
  ground->vertices.push_back(Mesh::Vertex(v3( 1.0f, 0.0f, -1.0f), v3(0.0f, 1.0f, 0.0f), v2(1.0f, 0.0f)));
  ground->vertices.push_back(Mesh::Vertex(v3(-1.0f, 0.0f, -1.0f), v3(0.0f, 1.0f, 0.0f), v2(0.0f, 0.0f)));
  ground->indices.push_back(0);
  ground->indices.push_back(1);
  ground->indices.push_back(2);
  ground->indices.push_back(0);
  ground->indices.push_back(2);
  ground->indices.push_back(3);
  ground->fill_buffers(device);

  model.position = v3();
  model.scale = v3(10.0f, 10.0f, 10.0f);
  model.y_axis_rotation = 0.0f;
  model.mesh = ground;

  renderer_data->models_to_render.push_back(model);
}

void render_model(Model *model)
{
  ID3D11DeviceContext *&device_context = renderer_data->resources.device_context;
  Window &window = renderer_data->window;
  Mesh &mesh = *model->mesh;
  Shader &shader = renderer_data->first_shader;
  Texture &texture = renderer_data->wizard_hat;

  // Set the vertex buffer to active in the input assembler so it can be rendered.
  ID3D11Buffer *buffers[] = {mesh.vertex_buffer};
  u32 strides[] = {sizeof(Mesh::Vertex)};
  u32 offsets[] = {0};
  u32 num_buffers = sizeof(buffers) / sizeof(buffers[0]);
  device_context->IASetVertexBuffers(0, num_buffers, buffers, strides, offsets);

  // Set the index buffer to active in the input assembler so it can be rendered.
  device_context->IASetIndexBuffer(mesh.index_buffer, DXGI_FORMAT_R32_UINT, 0);

  // Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
  device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


  // Set the vertex input layout.
  device_context->IASetInputLayout(shader.layout);

  // Set the vertex and pixel shaders that will be used to render this triangle.
  device_context->VSSetShader(shader.vertex_shader, NULL, 0);
  device_context->PSSetShader(shader.pixel_shader, NULL, 0);


  // World matrix
  mat4 scale_matrix = make_scale_matrix(model->scale);
  mat4 rotation_matrix = make_y_axis_rotation_matrix(deg_to_rad(model->y_axis_rotation));
  mat4 translation_matrix = make_translation_matrix(model->position);
  mat4 world_m_model = translation_matrix * rotation_matrix * scale_matrix;


  // View matrix
  // w
  // This is looking away from the target
  Camera &camera = renderer_data->camera;
  v3 camera_to_target = get_camera_to_target(1.0f, camera.latitude, camera.longitude);
  v3 target_axis = -camera_to_target;
  // u
  v3 right_axis = cross(WORLD_UP_VECTOR, target_axis);
  if(length(right_axis) == 0.0f) right_axis = v3(1.0f, 0.0f, 0.0f);
  right_axis = unit(right_axis);
  // v
  v3 up_axis = cross(target_axis, right_axis);
  up_axis = unit(up_axis);
  mat4 view_mat =
  {
    right_axis.x,  right_axis.y,  right_axis.z,  -dot(right_axis, camera.position),
    up_axis.x,     up_axis.y,     up_axis.z,     -dot(up_axis, camera.position),
    target_axis.x, target_axis.y, target_axis.z, -dot(target_axis, camera.position),
    0.0f, 0.0f, 0.0f, 1.0f,
  };
  mat4 view_m_world = view_mat;



  // Projection matrix
  f32 near_plane = 0.5f;
  f32 far_plane = 100.0f;
  f32 zoom = 1.0f;

  // Create an orthographic projection matrix for 2D rendering.
  D3DXMATRIX d3d_ortho;
  D3DXMatrixOrthoRH(&d3d_ortho, zoom * window.aspect_ratio, zoom, near_plane, far_plane);

  // Create the projection matrix for 3D rendering.
  f32 fov = deg_to_rad(60.0f) * zoom;
  //D3DXMATRIX d3d_persp;
  //D3DXMatrixPerspectiveFovRH(&d3d_persp, fov, window.aspect_ratio, near_plane, far_plane);
  //
  // Distance from the near plane (must be positive)
  f32 n = near_plane;
  // Distance from the far plane (must be positive)
  f32 f = far_plane;
  fov = deg_to_rad(60) * zoom;
  //f32 r = -(f + n) / (f - n);
  //f32 s = -(2 * n * f) / (f - n);
  f32 r = f / (n - f);
  f32 s = r * n;
  mat4 persp = 
  {
    (f32)(1.0f / tan(fov / 2.0f)) / window.aspect_ratio, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f / tan(fov / 2.0f), 0.0f, 0.0f,
    0.0f, 0.0f, r, s,
    0.0f, 0.0f, -1.0f, 0.0f
  };
  mat4 clip_m_view = persp;

  set_shader_paramters(device_context, shader.matrix_buffer, &world_m_model, &view_m_world, &clip_m_view, texture.texture_resource);

  // Set the sampler state in the pixel shader.
  device_context->PSSetSamplers(0, 1, &texture.sample_state);

  // Render the triangle.
  device_context->DrawIndexed(mesh.indices.size(), 0, 0);
}

void render()
{
  // Clear the back buffer.
  renderer_data->resources.device_context->ClearRenderTargetView(renderer_data->resources.render_target_view, renderer_data->window.background_color);

  // Clear the depth buffer.
  renderer_data->resources.device_context->ClearDepthStencilView(renderer_data->resources.depth_stencil_view, D3D11_CLEAR_DEPTH, 1.0f, 0);



  for(u32 i = 0; i < renderer_data->models_to_render.size(); i++)
  {
    render_model(&renderer_data->models_to_render[i]);
  }




  // Present the back buffer to the screen since rendering is complete.
  if(renderer_data->window.vsync)
  {
    // Lock to screen refresh rate.
    renderer_data->resources.swap_chain->Present(1, 0);
  }
  else
  {
    // Present as fast as possible.
    renderer_data->resources.swap_chain->Present(0, 0);
  }
}

void shutdown()
{
  // Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
  if(renderer_data->resources.swap_chain)
  {
    renderer_data->resources.swap_chain->SetFullscreenState(false, NULL);
  }

  if(renderer_data->resources.raster_state)
  {
    renderer_data->resources.raster_state->Release();
  }

  if(renderer_data->resources.depth_stencil_view)
  {
    renderer_data->resources.depth_stencil_view->Release();
  }

  if(renderer_data->resources.depth_stencil_state)
  {
    renderer_data->resources.depth_stencil_state->Release();
  }

  if(renderer_data->resources.depth_stencil_buffer)
  {
    renderer_data->resources.depth_stencil_buffer->Release();
  }

  if(renderer_data->resources.render_target_view)
  {
    renderer_data->resources.render_target_view->Release();
  }

  if(renderer_data->resources.device_context)
  {
    renderer_data->resources.device_context->Release();
  }

  if(renderer_data->resources.device)
  {
    renderer_data->resources.device->Release();
  }

  if(renderer_data->resources.swap_chain)
  {
    renderer_data->resources.swap_chain->Release();
  }

#if 0

  // Release the matrix constant buffer.
  if(matrix_buffer)
  {
    matrix_buffer->Release();
  }

  // Release the layout.
  if(layout)
  {
    layout->Release();
  }

  // Release the pixel shader.
  if(pixel_shader)
  {
    pixel_shader->Release();
  }

  // Release the vertex shader.
  if(vertex_shader)
  {
    vertex_shader->Release();
  }

  // Release the texture resource.
  for(s32 i = 0; i < sizeof(textures) / sizeof(textures[0]); i++)
  {
    if(textures[i])
    {
      textures[i]->Release();
    }
  }
#endif
}



v3 *camera_position() { return &renderer_data->camera.position; }
float *camera_latitude() { return &renderer_data->camera.latitude; }
float *camera_longitude() { return &renderer_data->camera.longitude; }

// In degrees
v3 get_camera_to_target(float radius, float latitude, float longitude)
{
  float x = radius * sin(deg_to_rad(longitude)) * cos(deg_to_rad(latitude));
  float z = radius * sin(deg_to_rad(longitude)) * sin(deg_to_rad(latitude));
  float y = radius * cos(deg_to_rad(longitude));
  return v3(x, y, z);
}

