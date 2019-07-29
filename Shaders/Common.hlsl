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
    uint     NormalMapIndex;
    uint     MatPad0;
    uint     MatPad1;
    uint     MatPad2;
};

struct InstanceData
{
    float4x4 World;
    float4x4 TexTransform ;
    uint MaterialIndex;
    uint InstancePad0;
    uint InstancePad1;
    uint InstancePad2;
};

TextureCube gCubeMap : register(t0);
Texture2D gShadowMap : register(t1);
Texture2D gTextureMaps[7] : register(t2);

//  Texture2D gDiffuseMap[6] : register(t0);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
 SamplerComparisonState gsamShadow : register(s6);

cbuffer cbPass : register(b0)
{
  float4x4 gView;
  float4x4 gInvView;
  float4x4 gProj;
  float4x4 gInvProj;
  float4x4 gViewProj;
  float4x4 gInvViewProj;
  float4x4 gShadowTransform;
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

//cbuffer cbPerObject : register(b1)
//{
//	float4x4 gWorld; 
//  float4x4 gTexTransform;
//  uint     MaterialIndex;
//	uint     ObjPad0;
//	uint     ObjPad1;
//	uint     ObjPad2;
//};


StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);
StructuredBuffer<InstanceData> gInstanceData : register(t1, space1);

float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
  // Uncompress each component from [0,1] to [-1,1].
  float3 normalT = 2.0f * normalMapSample - 1.0f;

  // Build orthonormal basis.
  float3 N = unitNormalW;
  float3 T = normalize(tangentW - dot(tangentW, N) * N);
  float3 B = cross(N, T);

  float3x3 TBN = float3x3(T, B, N);

  // Transform from tangent space to world space.
  float3 bumpedNormalW = mul(normalT, TBN);
  return bumpedNormalW;
}

float CalcShadowFactor(float4 shadowPosH)
{
    // Complete projection by doing division by w.
    shadowPosH.xyz /= shadowPosH.w;

    // Depth in NDC space.
    float depth = shadowPosH.z;

    uint width, height, numMips;
    gShadowMap.GetDimensions(0, width, height, numMips);

    // Texel size.
    float dx = 1.0f / (float)width;

    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx,  -dx), float2(0.0f,  -dx), float2(dx,  -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx,  +dx), float2(0.0f,  +dx), float2(dx,  +dx)
    };

    [unroll]
    for(int i = 0; i < 9; ++i)
    {
        percentLit += gShadowMap.SampleCmpLevelZero(gsamShadow,
            shadowPosH.xy + offsets[i], depth).r;
    }
    
    return percentLit / 9.0f;
}