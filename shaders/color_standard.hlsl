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
  float3 PosW : POSITION;
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
  return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
  float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinear, pin.TexC) * DiffuseAlbedo;

  pin.Normal = normalize(pin.Normal);

  // Vector from point being lit to eye. 
  float3 toEyeW = normalize(EyePosition - pin.PosW);

// Indirect lighting.
  float4 ambient = AmbientLight*diffuseAlbedo;

  const float shininess = 1.0f - Roughness;
  Material mat = { diffuseAlbedo, FresnelR0, shininess };
  float3 shadowFactor = 1.0f;

  float3 direct_light = ComputeDirectLight(Lights[0], toEyeW, pin.Normal, mat);
  //  float4 directLight = 

  float4 litColor = ambient + float4(direct_light, 1.0f);
  // float4 litColor = float4(Lights[0].Strength, 1.0f);
  // float4 litColor = float4(EyePosition, 1.0f);

  // float4 litColor = AmbientLight;

  // // Common convention to take alpha from diffuse material.
  // litColor.a = diffuseAlbedo.a;
  // float4 litColor = diffuseAlbedo;
  return litColor;
}