#include "Common.hlsl"

struct VertexIn {
    float3 PosL : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
};

// struct VertexOut {
//     float4 PosL : SV_POSITION;
//     float3 PosW : POSITION;
//     float3 Normal : NORMAL;
//     float2 TexCoord : TEXCOORD;
// };

struct VertexOut
{
	  float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};

VertexOut SkyVS(VertexIn vin, uint instanceID : SV_InstanceID) {
    VertexOut vout = (VertexOut)0.0f;

    InstanceData instData = gInstanceData[instanceID];

    //// Use local vertex position as cubemap lookup vector.
    vout.PosL = vin.PosL;

    //// Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), instData.World);

    //// Always center sky about camera.
    posW.xyz += gEyePosW;

    //// Set z = w so that z/w = 1 (i.e., skydome always on far plane).
    vout.PosH = mul(posW, gViewProj).xyww;

    return vout;
}

float4 SkyPS(VertexOut pin) : SV_TARGET
{
    //  return float4(1.0f, 0.0f, 0.0f, 1.0f);

     return gCubeMap.Sample(gsamLinearWrap, pin.PosL);
}