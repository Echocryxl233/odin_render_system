#include "common.hlsl"

struct VertexIn {
    float3 PosL : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut {
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};

TextureCube skyCubeMap : register(t0);
SamplerState gsamLinearWrap : register(s0);

VertexOut VS(VertexIn vin) {
    VertexOut vout = (VertexOut)0.0f;
    vout.PosL = vin.PosL;

    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

    posW.xyz += EyePosition;

    vout.PosH = mul(posW, ViewProj).xyww;
    return vout;
}

float4 PS(VertexOut pin) : SV_TARGET {
    return skyCubeMap.Sample(gsamLinearWrap, pin.PosL);
}