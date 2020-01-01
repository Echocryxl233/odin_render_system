#include "common.hlsl"

Texture2D    gDiffuseMap : register(t0);
Texture2D    gDiffuseMap2 : register(t1);
TextureCube skyCubeMap : register(t0, space2);

SamplerState gsamLinear  : register(s0);

struct VertexIn {
  float3 PosL : POSITION;
  float3 Normal : NORMAL;
  float2 TexC : TEXCOORD;
};

struct VertexOut
 {
  float4 Position : SV_POSITION;
  float3 Normal : NORMAL;
  float3 PosW : POSITION0;
  float4 SsaoPosH : POSITION1;
  float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin) {
  VertexOut vout;
  float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
  vout.PosW = posW.xyz;

  vout.Position = mul(posW, ViewProj);
  //  vout.Color = vin.Color;
  float4 Normal = mul(float4(vin.Normal, 0.0f), gWorld);
  vout.Normal = normalize(Normal.xyz);
  // vout.Normal = mul(vin.Normal, (float3x3)gWorld);
  float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), MatTransform);
  vout.TexC = texC.xy;

  vout.SsaoPosH = mul(posW, ViewProjTex);
  return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
  float4 diffuse_albedo = gDiffuseMap.Sample(gsamLinear, pin.TexC) * DiffuseAlbedo;
  // float4 diffuse_albedo = gSsapMap.Sample(gsamLinear, pin.TexC) * DiffuseAlbedo;

  pin.Normal = normalize(pin.Normal);

  // Vector from point being lit to eye. 
  float3 toEyeW = normalize(EyePosition - pin.PosW);
  
  float ambient_access = 1.0f;
  // pin.SsaoPosH /= pin.SsaoPosH.w;
  // ambient_access = gDiffuseMap2.SampleLevel(gsamLinear, pin.SsaoPosH.xy, 0.0f).r;

#ifdef SSAO
  pin.SsaoPosH /= pin.SsaoPosH.w;
  ambient_access = gSsapMap.SampleLevel(gsamLinear, pin.SsaoPosH.xy, 0.0f).r;
#endif

// Indirect lighting.
  float4 ambient = AmbientLight * diffuse_albedo * ambient_access;

  const float shininess = 1.0f - Roughness;
  Material mat = { diffuse_albedo, FresnelR0, shininess };
  float3 shadowFactor = 1.0f;

   // float3 direct_light = ComputeDirectLight(Lights[0], toEyeW, pin.Normal, mat);

  float3 point_light = 0.0f;
  int i=0;
#if NUM_POINT_LIGHTS
  // point_light += ComputePointLight(Lights[i], pin.PosW, toEyeW, pin.Normal, mat);
  for (i=0; i<NUM_POINT_LIGHTS; ++i) {
    point_light += ComputePointLight(Lights[i], pin.PosW, toEyeW, pin.Normal, mat);
  }
#endif

  float4 litColor = float4(point_light, 0.0f);

  float3 reflection_dir = reflect(-toEyeW, pin.Normal);

  float4 reflection_color = skyCubeMap.Sample(gsamLinear, reflection_dir);
  float3 reflect_factor = SchlickFresnel(FresnelR0, pin.Normal, reflection_dir);
  float3 env_light = shininess * reflect_factor * reflection_color;

  litColor.rgb += env_light.xyz;
  litColor.a = diffuse_albedo.a;

  return litColor;
}