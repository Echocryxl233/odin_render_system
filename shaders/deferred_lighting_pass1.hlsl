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

VertexOut VS(VertexIn vin)  {
    VertexOut vout = (VertexOut)0.0f;

    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    vout.Position = mul(posW, ViewProj);

    float4 normal = mul(float4(vin.Normal, 0.0f), gWorld);
    vout.Normal = normalize(normal.xyz);
    vout.NormalH = mul(normal, ViewProj); 

    float4 texcoord = mul(float4(vin.TexC, 1.0f, 1.0f), MatTransform);
    vout.TexC = texcoord.xy;

    return vout;
}

float4 PS(VertexOut pin) : SV_TARGET { // 
    float4 pout = (float4)0.0f;

    pout.xyz = pin.Normal * 0.5f + (float3)0.5f;
    pout.w = Roughness;

    return pout;

}

