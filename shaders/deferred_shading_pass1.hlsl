#include "common.hlsl"

// cbuffer cbPerObject : register(b0) {
//   float4x4 gWorld;
// };

Texture2D    gDiffuseMap : register(t0);

SamplerState gsamLinear  : register(s0);

struct VertexIn {
    float3 PosL : POSITION;
    float3 Normal : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut {
    float4 Position : SV_POSITION;
    float3 PosW : POSITION0;
    float3 Normal : NORMAL;
    float4 NormalH : POSITION1;
    float2 TexC : TEXCOORD;
};

struct PixelOut {
    float4 Position : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float4 Albedo : SV_TARGET2;
};

VertexOut VS(VertexIn vin) {
    VertexOut vout = (VertexOut)0.0f;

    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    vout.Position = mul(posW, ViewProj);

    float4 normal = mul(float4(vin.Normal, 0.0f), gWorld);
    vout.Normal = normal.xyz;
    vout.NormalH = mul(normal, ViewProj); 

    float4 texcoord = mul(float4(vin.TexC, 1.0f, 1.0f), MatTransform);
    vout.TexC = texcoord.xy;

    return vout;
}

PixelOut PS(VertexOut pin) : SV_TARGET { // 
    PixelOut pout = (PixelOut)0.0f;
    
    float position_w = length(pin.PosW);
    float3 position = normalize(pin.PosW);
    position = float3(0.5f, 0.5f, 0.5f) + position*0.5f;
    pout.Position = float4(position, position_w/100.0f);   //  * 1000.0f

    float normal_w = length(pin.Normal);
    float3 normal = pin.Normal / normal_w;
    normal = 0.5f + normal * 0.5f;
    pout.Normal = float4(normal, normal_w);

    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinear, pin.TexC); // DiffuseAlbedo;   // gDiffuseMap.Sample(gsamLinear, pin.TexC); // * 
    pout.Albedo = float4(diffuseAlbedo.xyz, 1.0f) * DiffuseAlbedo ;
    return pout;
}

