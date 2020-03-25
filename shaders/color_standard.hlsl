#include "common.hlsl"

Texture2D    gDiffuseMap : register(t0);
Texture2D    gDiffuseMap2 : register(t1);
TextureCube  skyCubeMap : register(t0, space2);
Texture2D    gSsapMap : register(t1, space2);
Texture2D    gShadowMap : register(t2, space2);


SamplerState samplerLinear  : register(s0);
SamplerComparisonState samplerShadow : register(s1);

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
  float4 ShadowPosH : POSITION2;
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
  vout.ShadowPosH = mul(posW, ShadowTransform);
  return vout;
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
        percentLit += gShadowMap.SampleCmpLevelZero(samplerShadow,
            shadowPosH.xy + offsets[i], depth).r;
    }
    
    return percentLit / 9.0f;
}

float4 PS(VertexOut pin) : SV_Target
{
  float4 diffuse_albedo = gDiffuseMap.Sample(samplerLinear, pin.TexC) * DiffuseAlbedo;

  pin.Normal = normalize(pin.Normal);

  // Vector from point being lit to eye. 
  float3 toEyeW = normalize(EyePosition - pin.PosW);
  
  float ambient_access = 1.0f;

#ifdef SSAO
  pin.SsaoPosH /= pin.SsaoPosH.w;
  ambient_access = gSsapMap.SampleLevel(samplerLinear, pin.SsaoPosH.xy, 0.0f).r;
#endif

// Indirect lighting.
  float4 ambient = AmbientLight * diffuse_albedo * ambient_access;

  const float shininess = 1.0f - Roughness;
  Material mat = { diffuse_albedo, FresnelR0, shininess };
  float3 shadowFactor = 1.0f;
  shadowFactor = CalcShadowFactor(pin.ShadowPosH);
   // float3 direct_light = ComputeDirectLight(Lights[0], toEyeW, pin.Normal, mat);


  float3 direct_radiance = 0.0f;
  int i=0;
#if NUM_DIR_LIGHTS
  for (i=0; i<NUM_DIR_LIGHTS; ++i) {
    direct_radiance += shadowFactor * ComputeDirectLight(Lights[i], toEyeW, pin.Normal, mat);
  }
#endif

  float3 point_light = 0.0f;

#if NUM_POINT_LIGHTS
  // point_light += ComputePointLight(Lights[i], pin.PosW, toEyeW, pin.Normal, mat);
  for (i=NUM_DIR_LIGHTS; i<NUM_POINT_LIGHTS; ++i) {
    point_light += ComputePointLight(Lights[i], pin.PosW, toEyeW, pin.Normal, mat);
  }
#endif

  float4 litColor = float4(direct_radiance + point_light, 0.0f);

  float3 reflection_dir = reflect(-toEyeW, pin.Normal);

  float4 reflection_color = skyCubeMap.Sample(samplerLinear, reflection_dir);
  float3 reflect_factor = SchlickFresnel(FresnelR0, pin.Normal, reflection_dir);
  float3 env_light = shininess * reflect_factor * reflection_color;

  litColor.rgb += env_light.xyz;
  litColor.a = diffuse_albedo.a;

  return litColor;
}


