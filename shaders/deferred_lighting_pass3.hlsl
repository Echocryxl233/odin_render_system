#include "common.hlsl"


Texture2D    gDiffuseMap : register(t0);
Texture2D    gSpecularMap : register(t1);

Texture2D    gScreenDiffuseMap : register(t0, space1);
Texture2D    gScreenSpecularMap : register(t1, space1);

// Texture2D gTextureMaps[10] : register(t2);

SamplerState gsamLinear  : register(s0);
// SamplerState gsamDepthMap : register(s1);

struct VertexIn {
    float3 PosL : POSITION;
    float3 Normal : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut {
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION0;
    float3 Normal : NORMAL;
    float4 NormalH : POSITION1;
    float2 TexC : TEXCOORD;
    float2 STexC : TEXCOORD1;       //  screen-space texture coordinate
};

VertexOut VS(VertexIn vin)  {
    VertexOut vout = (VertexOut)0.0f;

    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    vout.PosH = mul(posW, ViewProj);

    float4 normal = mul(float4(vin.Normal, 1.0f), gWorld);
    vout.Normal = normalize(normal.xyz);
    vout.NormalH = mul(normal, ViewProj); 

    float4 texcoord = mul(float4(vin.TexC, 1.0f, 1.0f), MatTransform);
    vout.TexC = texcoord.xy;
    float2 sTexC = vout.PosH.xy / vout.PosH.w;
    vout.STexC = float2((sTexC.x+1.0f) * 0.5f, (1.0f - sTexC.y) * 0.5f );

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 screen_diffuse = gScreenDiffuseMap.Sample(gsamLinear, pin.STexC.xy);
    float4 screen_specular = gScreenSpecularMap.Sample(gsamLinear, pin.STexC.xy);

    float4 texture_diffuse = gDiffuseMap.Sample(gsamLinear, pin.TexC);

    float3 diffuse = (texture_diffuse + screen_specular).xyz * screen_diffuse.xyz;
    
    return float4(diffuse, texture_diffuse.a);
}