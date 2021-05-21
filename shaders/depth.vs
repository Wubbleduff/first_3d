
cbuffer MatrixBuffer
{
  matrix clip_m_model;
};

struct VSInput
{
  float3 position : POSITION;
  float3 normal : NORMAL;
  float2 tex : TEXCOORD0;
};

struct VSOutput
{
  float4 position : SV_POSITION;
};

VSOutput depth_vertex_shader(VSInput input)
{
  VSOutput output;

  float4 modelspace_vertex_position;
  modelspace_vertex_position.xyz = input.position;
  modelspace_vertex_position.w = 1.0f;

  // Calculate the position of the vertex against the world, view, and projection matrices.
  output.position = mul(modelspace_vertex_position, clip_m_model);

  return output;
}

