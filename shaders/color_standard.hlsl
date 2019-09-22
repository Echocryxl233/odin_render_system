// struct Light
// {
//   float3 Strength;
//   float FalloffStart;
//   float3 Direction;
//   float FalloffEnd;
//   float3 Position;
//   float SpotPower;
// };

#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

#include "lighting_util.hlsl"

Texture2D    gDiffuseMap : register(t0);
Texture2D    gDiffuseMap2 : register(t1);
// Texture2D    gSsapMap : register(t0, space2);
SamplerState gsamLinear  : register(s0);

cbuffer cbPerObject : register(b0) {
  float4x4 gWorld;
};

cbuffer cbMaterial : register(b1) {
  float4 DiffuseAlbedo;
  float3 FresnelR0;
  float Roughness;
  float4x4 MatTransform;
};

cbuffer cbPassConstant : register(b2) {
  float4x4 View;
  float4x4 InvView;
  float4x4 Proj;
  float4x4 InvProj;
  float4x4 ViewProj;
  float4x4 ViewProjTex;
  float3 EyePosition;
  float  Padding1;
  float4 AmbientLight;
  Light Lights[1];
};

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
  float4 Normal = mul(float4(vin.Normal, 1.0f), gWorld);
  vout.Normal = Normal.xyz;
  float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), MatTransform);
  vout.TexC = texC.xy;

  vout.SsaoPosH = mul(posW, ViewProjTex);
  return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
  float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinear, pin.TexC) * DiffuseAlbedo;
  // float4 diffuseAlbedo = gSsapMap.Sample(gsamLinear, pin.TexC) * DiffuseAlbedo;

  pin.Normal = normalize(pin.Normal);

  // Vector from point being lit to eye. 
  float3 toEyeW = normalize(EyePosition - pin.PosW);
  
  float ambient_access = 1.0f;
  pin.SsaoPosH /= pin.SsaoPosH.w;
  ambient_access = gDiffuseMap2.SampleLevel(gsamLinear, pin.SsaoPosH.xy, 0.0f).r;

#ifdef SSAO
  pin.SsaoPosH /= pin.SsaoPosH.w;
  ambient_access = gSsapMap.SampleLevel(gsamLinear, pin.SsaoPosH.xy, 0.0f).r;
#endif

// Indirect lighting.
  float4 ambient = AmbientLight * diffuseAlbedo * ambient_access;

  const float shininess = 1.0f - Roughness;
  Material mat = { diffuseAlbedo, FresnelR0, shininess };
  float3 shadowFactor = 1.0f;

  float3 direct_light = ComputeDirectLight(Lights[0], toEyeW, pin.Normal, mat);
  //  float4 directLight = 

  float4 litColor = ambient + float4(direct_light, 1.0f);

  return litColor;
}