cbuffer global_buffer
{
  matrix world_m_model;
  matrix view_m_world;
  matrix clip_m_view;
};

struct VSInput
{
  float3 position : POSITION;
  float3 normal : NORMAL;
  float2 tex : TEXCOORD;
};

struct VSOutput
{
  float4 position : SV_POSITION;
  float3 modelspace_position : VIEW_POSITION;
};

// Vertex shader
VSOutput skybox_vertex_shader(VSInput input)
{
  VSOutput output;

  float4 modelspace_vertex_position;
  modelspace_vertex_position.xyz = input.position;
  modelspace_vertex_position.w = 1.0f;

  // Calculate the position of the vertex against the world, view, and projection matrices.
  output.position = mul(modelspace_vertex_position, world_m_model);
  output.position = mul(output.position, view_m_world);
  output.position = mul(output.position, clip_m_view);

  output.modelspace_position = input.position;
  
  
  return output;
}
