#include "common.hlsl"

Texture2D    gPositionMap : register(t0);
Texture2D    gNormalMap : register(t1);
Texture2D    gColorMap : register(t2);
Texture2D    gDepthMap : register(t3);

// Texture2D gTextureMaps[10] : register(t2);

SamplerState gsamLinear  : register(s0);
SamplerState gsamDepthMap : register(s1);

struct VertexIn
{
  float3 PosL : POSITION;
  float3 Normal : NORMAL;
  float2 TexC : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
    // Already in homogeneous clip space.
  vout.PosH = float4(vin.PosL, 1.0f);
	vout.TexC = vin.TexC;
  return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
  float4 sample_position = gPositionMap.Sample(gsamLinear, pin.TexC);
  float3 position = sample_position.xyz;
  position = (position - 0.5f) * 2.0f * sample_position.w * 100.0f ;

  float4 sample_normal = gNormalMap.Sample(gsamLinear, pin.TexC);
  float3 normal = sample_normal.xyz;
  normal = (normal - 0.5f) * 2.0f ;

  float4 albedo = gColorMap.Sample(gsamLinear, pin.TexC);
  float depth = gDepthMap.SampleLevel(gsamDepthMap, pin.TexC, 0.0f).r;

  float4 ndc = float4(pin.TexC.x*2.0f -1.0f, 1.0f - pin.TexC.y*2.0f , depth, 1.0f);
  float4 worldPos = mul(ndc, InvViewProj);
  worldPos/= worldPos.w;

  float3 result = 0.0f;
  int i=0; 

#if NUM_POINT_LIGHTS
  for (i=0; i<NUM_POINT_LIGHTS; ++i) {
    float3 light_position = Lights[i].Position;
    // float3 light_direction = light_position-position;
    float3 light_direction = light_position-worldPos.xyz;
    float d = length(light_direction );

    if (d <= Lights[i].FalloffEnd && depth < 1.0f) {
        light_direction /= d;
        float nDotL = max(0.0f, dot(normalize(normal), light_direction));
        float3 light_strength = Lights[i].Strength ; 
        float attenuation = CalculateAttenuation(d, Lights[i].FalloffStart, Lights[i].FalloffEnd);
        light_strength *= attenuation;

        result += light_strength * albedo;
    }
  }
#endif
  return float4(result.xyz, albedo.a);
}