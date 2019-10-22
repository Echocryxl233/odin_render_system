cbuffer cbSetting : register(b0) {
    float PlaneInFocus;
    //  float ProjTex;
    float Znear;
    float Zfar;
    float CoC;
    float Ratio;
    float padding1;
    float padding2;
    float padding3;
}

cbuffer cbObject : register(b1) {
    float4x4 World;
}

cbuffer cbPass : register(b2) {
    float4x4 View;
    float4x4 Proj;
    float4x4 ViewProj;
}

Texture2D depthMap : register(t0);
Texture2D blurMap : register(t1);
Texture2D colorMap : register(t2);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);

struct VertexIn {
    float3 Position : POSITION;
    float2 TexC : TEXCOORD;
};

struct VertexOut {
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin) {
    VertexOut vout = (VertexOut)0.0f;
    float4 posW = mul(float4(vin.Position, 1.0f), World);

    vout.PosW = posW.xyz;
    vout.PosH = mul(posW, ViewProj);
    vout.TexC = vin.TexC;

    return vout;
}

float4 PS(VertexOut pin) : SV_TARGET
{
    float4 blur_color = blurMap.Sample(gsamPointClamp, pin.TexC.xy);
    float4 origin_color = colorMap.Sample(gsamPointClamp, pin.TexC.xy);\
    
    float z = depthMap.SamplerLevel(gsamPointClamp, pin.TextC.xy, 0.0f).r;

    float distance = Zfar * Znear / (z * (Zfar - Znear) - Zfar)

    if (PlaneInFocus*(1+Ratio) > distance && PlaneInFocus*(1-Ratio) < distance) {
        return blue_color;
    }

    return origin_color;
}