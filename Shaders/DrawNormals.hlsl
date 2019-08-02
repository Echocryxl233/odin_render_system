//***************************************************************************************
// DrawNormals.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 0
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

// Include common HLSL code.
#include "Common.hlsl"

struct VertexIn
{
	float3 PosL    : POSITION;
  float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
	float3 TangentU : TANGENT;
};

struct VertexOut
{
	float4 PosH     : SV_POSITION;
  float3 NormalW  : NORMAL;
	float3 TangentW : TANGENT;
	float2 TexC     : TEXCOORD;

  nointerpolation uint MatIndex  : MATINDEX;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
  VertexOut vout = (VertexOut)0.0f;

  InstanceData instData = gInstanceData[instanceID];
  float4x4 world = instData.World;
  uint matIndex = instData.MaterialIndex;
  float4x4 texTransform = instData.TexTransform;

  // Fetch the material data.
  MaterialData matData = gMaterialData[matIndex];

  // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
  vout.NormalW = mul(vin.NormalL, (float3x3)world);
  vout.TangentW = mul(vin.TangentU, (float3x3)world);

  // Transform to homogeneous clip space.
  float4 posW = mul(float4(vin.PosL, 1.0f), world);
  vout.PosH = mul(posW, gViewProj);
	
  // Output vertex attributes for interpolation across triangle.
  float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), texTransform);
  vout.TexC = mul(texC, matData.MatTransform).xy;

  vout.MatIndex = matIndex;
	
  return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
  // Fetch the material data.
  MaterialData matData = gMaterialData[pin.MatIndex];
  float4 diffuseAlbedo = matData.DiffuseAlbedo;
  uint diffuseMapIndex = matData.DiffuseMapIndex;
  uint normalMapIndex = matData.NormalMapIndex;
	
  // Dynamically look up the texture in the array.
  diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);

#ifdef ALPHA_TEST
  // Discard pixel if texture alpha < 0.1.  We do this test as soon 
  // as possible in the shader so that we can potentially exit the
  // shader early, thereby skipping the rest of the shader code.
  clip(diffuseAlbedo.a - 0.1f);
#endif

  // Interpolating normal can unnormalize it, so renormalize it.
  pin.NormalW = normalize(pin.NormalW);
	
  // NOTE: We use interpolated vertex normal for SSAO.

  // Write normal in view space coordinates
  float3 normalV = mul(pin.NormalW, (float3x3)gView);
  return float4(normalV, 0.0f);
}


