#include "common.hlsl"

Texture2D    gMaterialMap : register(t0);
Texture2D    gNormalMap : register(t1);
Texture2D    gColorMap : register(t2);
Texture2D    gDepthMap : register(t3);
TextureCube    skyCubeMap : register(t4);

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
  float4 EyePosH : POSITION0;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
    // Already in homogeneous clip space.
  vout.PosH = float4(vin.PosL, 1.0f);

  // float3 eye_world_pos = EyePosition + float3(0.0f, 0.0f, 1.0f);
  // float4 posH = mul(float4(eye_world_pos, 1.0f), ViewProj);
  // vout.EyePosH = posH;

	vout.TexC = vin.TexC;
  return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
  float4 sample_normal = gNormalMap.Sample(gsamLinear, pin.TexC);
  float3 normal = sample_normal.xyz;
  normal = (normal - 0.5f) * 2.0f ;

  float4 albedo = gColorMap.Sample(gsamLinear, pin.TexC);
  float depth = gDepthMap.SampleLevel(gsamDepthMap, pin.TexC, 0.0f).r;

  float4 ndc = float4(pin.TexC.x*2.0f -1.0f, 1.0f - pin.TexC.y*2.0f , depth, 1.0f);   //  directX y 0 is at bottom and 1 is the top
  float4 worldPos = mul(ndc, InvViewProj);
  worldPos/= worldPos.w;

  // if (depth >= 1.0f) {

  //   return  skyCubeMap.Sample(gsamLinear, float3(pin.PosH.x - 0.5f, 0.5f-pin.PosH.y, 1.0f) );
  //   // return float4(1.0f, 0.0f, 1.0f, 1.0f);
  // }

  float3 result = 0.0f;
  int i=0; 

  float3 direct_radiance = 0.0f;
  float3 result_diffuse = 0.0f;
  float3 result_specular = 0.0f;
  float3 light_direction = 0.0f;
  float nDotL = 0.0f;
  float vDotR = 0.0f;
  float3 r = 0.0f;
  float3 half_vector  = 0.0f;
  float3 light_strength = 0.0f;
  float3 specular_albedo = 0.0f;

  float3 to_eye = EyePosition - worldPos.xyz;

#if NUM_DIR_LIGHTS
  for (i=0; i<NUM_DIR_LIGHTS; ++i) {
    // light_direction = normalize(Lights[i].Direction);
    light_strength = Lights[i].Strength ; 
    light_direction = -Lights[i].Direction;

    nDotL = max(0.0f, dot(normal, light_direction));

    light_strength = light_strength * nDotL;

    r = normalize(reflect(-light_direction, normal));
    float rDotV = max(0.0f, dot(r, normalize(to_eye)));

    specular_albedo = pow(rDotV, 16);
    result += (specular_albedo + albedo) * light_strength;
  }
#endif


#if NUM_POINT_LIGHTS
  for (; i<NUM_POINT_LIGHTS; ++i) {
    float3 light_position = Lights[i].Position;
    // float3 light_direction = light_position-position;
    light_direction = light_position-worldPos.xyz;
    float d = length(light_direction );

    if (d <= Lights[i].FalloffEnd && depth < 1.0f) {
        light_direction /= d;
        nDotL = max(0.0f, dot(normalize(normal), light_direction));
        light_strength = Lights[i].Strength ; 
        float attenuation = CalculateAttenuation(d, Lights[i].FalloffStart, Lights[i].FalloffEnd);
        light_strength *= attenuation;

        result += light_strength * albedo;
    }
  }
#endif

  float4 material = gMaterialMap.Sample(gsamLinear, pin.TexC);
  
  float3 reflection_dir = reflect(-to_eye, normal);
  float3 reflection_color = skyCubeMap.Sample(gsamLinear, reflection_dir);
  float reflect_factor = SchlickFresnel(material.xyz, normal, reflection_dir);

  float3 env_light = (1.0f-material.w) * reflect_factor * reflection_color;

  result += env_light;

  return float4(result.xyz, albedo.a);
}