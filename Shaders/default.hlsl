//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************
 
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 1
#endif

#include "LightUtil.hlsl"

struct MaterialData {
  float4   DiffuseAlbedo;
	float3   FresnelR0;
	float    Roughness;
	float4x4 MatTransform;
	uint     DiffuseMapIndex;
	uint     MatPad0;
	uint     MatPad1;
	uint     MatPad2;
};

//Texture2D gDiffuseMap[4] : register(t0);

Texture2D    gDiffuseMap : register(t0);
SamplerState gsamLinear  : register(s0);

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld; 
  float4x4 gTexTransform;
  uint     MaterialIndex;
	uint     ObjPad0;
	uint     ObjPad1;
	uint     ObjPad2;
};

cbuffer cbPass : register(b1)
{
  float4x4 gView;
  float4x4 gInvView;
  float4x4 gProj;
  float4x4 gInvProj;
  float4x4 gViewProj;
  float4x4 gInvViewProj;
  float3 gEyePosW;
  float cbPerObjectPad1;
  float2 gRenderTargetSize;
  float2 gInvRenderTargetSize;
  float gNearZ;
  float gFarZ;
  float gTotalTime;
  float gDeltaTime;
  float4 gAmbientLight;

  Light gLights[MaxLights];
};

StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

//cbuffer cbMaterial : register(b2)
//{
//  float4 gDiffuseAlbedo;
//  float3 gFresnelR0;
//  float  gRoughness;
//	float4x4 gMatTransform;
//};

struct VertexIn
{
	float3 PosL  : POSITION;
  float3 Normal : NORMAL; // normal model
  float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
  float3 PosW  : POSITION;
  float3 Normal : NORMAL; //  normal world
  float2 TexCoord : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
  MaterialData matData = gMaterialData[MaterialIndex];

	// Transform to homogeneous clip space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    vout.PosH = mul(posW, gViewProj);
	
	// Just pass vertex color into the pixel shader.
    //  vout.Color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    vout.Normal = mul(vin.Normal, (float3x3)gWorld);
    float4 texCoord = mul(float4(vin.TexCoord, 0.0f, 1.0f), gTexTransform);
    //  float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    //  vout.TexCoord = texCoord.xy;
    vout.TexCoord = mul(texCoord, matData.MatTransform).xy;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{

 // //  uint deffuseIndex = 
 //   float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinear, pin.TexCoord) * gDiffuseAlbedo;
 // //  float4 diffuseAlbedo = gDiffuseAlbedo;

 // pin.Normal = normalize(pin.Normal);

 //   // Vector from point being lit to eye. 
 //   float3 toEyeW = normalize(gEyePosW - pin.PosW);

	//// Indirect lighting.
 //   float4 ambient = gAmbientLight*diffuseAlbedo;

 //   const float shininess = 1.0f - gRoughness;
 //   Material mat = { diffuseAlbedo, gFresnelR0, shininess };
 //   float3 shadowFactor = 1.0f;
 //   float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
 //       pin.Normal, toEyeW, shadowFactor);

 //   float4 litColor = ambient + directLight;

 //   // Common convention to take alpha from diffuse material.
 //   litColor.a = diffuseAlbedo.a;

 //     return litColor;

  return float4(1.0f, 1.0f, 1.0f, 1.0f);
}


