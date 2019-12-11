#include "common.hlsl"


Texture2D    gNormalMap : register(t0);
Texture2D    gDepthMap : register(t1);

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

struct PixelOut {
    float4 Diffuse : SV_TARGET1;
    float4 Specular : SV_TARGET2;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
    // Already in homogeneous clip space.
  vout.PosH = float4(vin.PosL, 1.0f);
	vout.TexC = vin.TexC;
  return vout;
}

PixelOut PS(VertexOut pin) : SV_Target
{
  PixelOut pout = (PixelOut)0.0f;
  float4 sample_normal = gNormalMap.Sample(gsamLinear, pin.TexC);
  float3 normal = sample_normal.xyz;
  normal = (normal - 0.5f) * 2.0f ;

  float roghness = sample_normal.w;
  float shiness = 1.0f - roghness;
  float m = shiness * 256.0f;

//   float4 albedo = gColorMap.Sample(gsamLinear, pin.TexC);
  float depth = gDepthMap.SampleLevel(gsamDepthMap, pin.TexC, 0.0f).r;

  float4 ndc = float4(pin.TexC.x*2.0f -1.0f, 1.0f - pin.TexC.y*2.0f , depth, 1.0f);   //  directX y 0 is at bottom and 1 is the top
  float4 worldPos = mul(ndc, InvViewProj);
  worldPos/= worldPos.w;

  float3 result_diffuse = 0.0f;
  float3 result_specular = 0.0f;
  int i=0; 

#if NUM_POINT_LIGHTS
  for (i=0; i<NUM_POINT_LIGHTS; ++i) {
    float3 light_position = Lights[i].Position;
//     // float3 light_direction = light_position-position;
    float3 light_direction = light_position-worldPos.xyz;
    float d = length(light_direction );

    if (d <= Lights[i].FalloffEnd && depth < 1.0f) {
        float nDotL = max(0.0f, dot(normalize(normal), light_direction));
        float3 light_strength = Lights[i].Strength ; 
        float attenuation = CalculateAttenuation(d, Lights[i].FalloffStart, Lights[i].FalloffEnd);
        light_strength *= attenuation;
        result_diffuse += light_strength * nDotL;


        float3 to_eye = EyePosition - worldPos.xyz;
        float3 half_vector = normalize(to_eye + light_direction);
        float3 specular_albedo = (8.0f + m) * pow(max(0.0f, dot(normal, half_vector)), m) / 8.0f;
        specular_albedo /= (1.0f + specular_albedo);
        result_specular += specular_albedo;
    }
  }
#endif
  // pout.Diffuse.xyz = float4(1.0f, 0.0f, 1.0f, 1.0f);
  
  pout.Diffuse.rgb = result_diffuse.xyz;
  pout.Diffuse.a = 1.0f;
  
  pout.Specular.rgb = result_specular;
  // pout.Specular.xyz = float4(1.0f, 0.0f, 1.0f, 1.0f);
  pout.Specular.a = 1.0f;
  return pout;
  // return gNormalMap.Sample(gsamLinear, pin.TexC);
}