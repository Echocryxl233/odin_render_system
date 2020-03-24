#include "lighting_util.hlsl"

#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 49
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif


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
  float4x4 InvViewProj;
  float4x4 ViewProjTex;
  float4x4 ShadowTransform;
  float3 EyePosition;
  float  Padding1;
  float ZNear;
  float ZFar;
  float Padding2;
  float padding3;
  float4 AmbientLight;
  Light Lights[MaxLights];
};



