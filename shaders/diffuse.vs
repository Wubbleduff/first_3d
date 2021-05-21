// VertexShader.vs

// Global variables
cbuffer MatrixBuffer
{
  matrix world_m_model;
  matrix view_m_world;
  matrix clip_m_view;
  matrix light_clip_m_model;
  float4 color;
  float4 light_position;
};

// Typedefs
struct VSInput
{
  float3 position : POSITION;
  float3 normal : NORMAL;
  float2 tex : TEXCOORD0;
};

struct VSOutput
{
  float4 position : SV_POSITION;
  float3 worldspace_position : WORLD_POSITION;
  float3 normal : NORMAL0;
  float2 tex : TEXCOORD0;
  float4 blend_color : COLOR;
  float3 worldspace_light_position : LIGHT_POSITION;
  float4 light_clipspace_position : LIGHT_CLIP_POSITION;
};

// Vertex shader
VSOutput diffuse_vertex_shader(VSInput input)
{
  VSOutput output;

  float4 modelspace_vertex_position;
  modelspace_vertex_position.xyz = input.position;
  modelspace_vertex_position.w = 1.0f;

  // Calculate the position of the vertex against the world, view, and projection matrices.
  output.position = mul(modelspace_vertex_position, world_m_model);
  output.position = mul(output.position, view_m_world);
  output.position = mul(output.position, clip_m_view);

  output.worldspace_position = mul(modelspace_vertex_position, world_m_model).xyz;

  // Normal
  output.normal = mul(float4(input.normal, 0.0f), world_m_model).xyz;
  output.normal = normalize(output.normal);
  
  // Tex coords
  output.tex = input.tex;

  output.blend_color = color;

  output.worldspace_light_position = light_position.xyz;


  output.light_clipspace_position = mul(modelspace_vertex_position, light_clip_m_model);


  return output;
}

