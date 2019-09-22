cbuffer cbObject : register (b0) {
    float4x4 gWorld;
};

cbuffer cbPass : register(b1) {
    float4x4 gProj;
    float4x4 gViewProj;
};

struct VertexIn {
    float3 Pos : POSITION;
    float3 NormalL : NORMAL;
};

struct VertexOut {
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
};

VertexOut VS(VertexIn vin) {
    VertexOut vout = (VertexOut)0.0f;
    float4 pos_world = mul(float4(vin.Pos, 1.0f), gWorld);
    vout.PosW = pos_world.xyz;

    vout.PosH = mul(pos_world, gViewProj);

    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

    return vout;
};

float4 PS(VertexOut pin) : SV_TARGET {
    float3 normalV = mul(pin.NormalW, (float3x3)gProj);
    return float4(normalV, 0.0f);
};