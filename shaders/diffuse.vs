// VertexShader.vs

// Global variables
cbuffer MatrixBuffer
{
  matrix world_m_model;
  matrix view_m_world;
  matrix clip_m_view;
  float4 color;
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
  float3 normal : NORMAL0;
  float2 tex : TEXCOORD0;
  float4 color : COLOR;
};

// Vertex shader
VSOutput ColorVertexShader(VSInput input)
{
  VSOutput output;

  float4 modelspace_vertex_position;
  modelspace_vertex_position.xyz = input.position;
  modelspace_vertex_position.w = 1.0f;

  // Calculate the position of the vertex against the world, view, and projection matrices.
  output.position = mul(modelspace_vertex_position, world_m_model);
  output.position = mul(output.position, view_m_world);
  output.position = mul(output.position, clip_m_view);

  // Normal
  output.normal = mul(input.normal, world_m_model);
  
  // Tex coords
  output.tex = input.tex;

  // Sprite color
  output.color = color;
  
  return output;
}

