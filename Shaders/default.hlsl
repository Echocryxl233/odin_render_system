//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************
 
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 1
#endif

#include "LightUtil.hlsl"

Texture2D    gDiffuseMap : register(t0);
SamplerState gsamLinear  : register(s0);

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld; 
  float4x4 gTexTransform;
};

cbuffer cbMaterial : register(b1)
{
  float4 gDiffuseAlbedo;
  float3 gFresnelR0;
  float  gRoughness;
	float4x4 gMatTransform;
};

cbuffer cbPass : register(b2)
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
	
	// Transform to homogeneous clip space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    vout.PosH = mul(posW, gViewProj);
	
	// Just pass vertex color into the pixel shader.
    //  vout.Color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    vout.Normal = mul(vin.Normal, (float3x3)gWorld);
    float4 texCoord = mul(float4(vin.TexCoord, 0.0f, 1.0f), gTexTransform);
    //  float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexCoord = texCoord.xy;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinear, pin.TexCoord) * gDiffuseAlbedo;
  //  float4 diffuseAlbedo = gDiffuseAlbedo;

  pin.Normal = normalize(pin.Normal);

    // Vector from point being lit to eye. 
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

	// Indirect lighting.
    float4 ambient = gAmbientLight*diffuseAlbedo;

    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.Normal, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    // Common convention to take alpha from diffuse material.
    litColor.a = diffuseAlbedo.a;

      return litColor;
    //  float3 c =  BlinnPhong(gLights[0].Strength, gLights[0].Direction, pin.Normal, toEyeW, mat);

    //  return float4(c, gDiffuseAlbedo.a);

    //  float3 c = ComputeDirectionalLight(gLights[0], mat, pin.Normal, toEyeW);
    //  return float4(c, gDiffuseAlbedo.a);
}


