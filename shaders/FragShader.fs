// FragShader.fs

// Globals
Texture2D shader_texture;
SamplerState sample_type;

// Typedef
struct PSInput
{
  float4 position : SV_POSITION;
  float3 normal : NORMAL0;
  float2 tex : TEXCOORD0;
  float4 color : COLOR; 
};

// Pixel shader
float4 ColorPixelShader(PSInput input) : SV_TARGET
{
  float4 texture_color;

  texture_color = shader_texture.Sample(sample_type, input.tex);

  //texture_color *= input.color;

  float3 light_position = float3(0.0f, 1.0f, 1.0f);
  float intensity = dot(light_position, input.normal);
  texture_color.xyz *= intensity;

  return texture_color;
}
