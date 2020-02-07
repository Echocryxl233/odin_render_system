cbuffer SSRConstant : register(b0) {
    float4x4 View;
    float4x4 InvView;
    float4x4 Proj;
    float4x4 InvProj;
};

Texture2D gNormalMap : register(t0);
Texture2D gDepthMap : register(t1);
Texture2D gColorMap : register(t2);    //  gInputMap in ssao_blur.hlsl

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);

static const float2 gTexCoods[6] = {
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f)
};


static const int gSampleCount = 14;


struct VertexOut {
    float4 PosH : SV_POSITION;
    float3 PosV : POSITION;
    float2 TexC : TEXCOORD;
};

VertexOut VS(uint vid : SV_VERTEXID) {
    VertexOut vout = (VertexOut)0.0f;
    vout.TexC = gTexCoods[vid];
    vout.PosH = float4(2.0f * vout.TexC.x - 1.0f, 1.0f - 2.0f * vout.TexC.y, 0.0f, 1.0f);   //  [0, 1] -> [-1, 1]
    // vout.PosH = float4(1.0f * vout.TexC.x - 0.5f, 0.5f - 1.0f * vout.TexC.y, 0.0f, 1.0f);   //  [0, 1] -> [-1, 1]
    float4 ph = mul(vout.PosH, InvProj);
    vout.PosV = ph.xyz / ph.w;
    return vout;
}


float4 PS(VertexOut pin) : SV_TARGET
{
    float3 world_normal = normalize(gNormalMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).xyz);
    float3 cs_normal = normalize(mul(world_normal, (float3x3)View));
    float pz = gDepthMap.SampleLevel(gsamDepthMap, pin.TexC, 0.0f).r;
    float4 cs_ray = float4(pin.PosH.xy, 1.0f, 1.0f);
    cs_ray = mul(cs_ray, InvProj);

    cs_ray /= cs_ray.w;

    float4 start = cs_ray * pz;

    float3 reflect_dir = normalize(reflect(start.xyz, cs_normal));

    float3 reflect_color = (float3)0.0f;

    float f=0;

    int i=0;
    [unroll]
    for (; i< 10; ++i) {
        float3 end = start.xyz + i*reflect_dir;
        float4 hit_pos = mul(float4(end, 1.0f), Proj);
        hit_pos /= hit_pos.w;

        if (hit_pos.x < -1.0f || hit_pos.y < -1.0f || hit_pos.x > 1.0f || hit_pos.y > 1.0f) {
            f = 0.7f;
            break;
        }

        float2 target_texcoord = float2(0.5f * hit_pos.x+0.5f, 0.5f - 0.5f * hit_pos.y);
        float surface_depth = gDepthMap.SampleLevel(gsamDepthMap, target_texcoord, 0.0f).r;

        if (surface_depth < hit_pos.z) {
            reflect_color = gColorMap.Sample(gsamLinearClamp, target_texcoord, 0.0f);
            f = (float)i / 10.0f;
            break;
        }
    }


    float3 color = gColorMap.Sample(gsamLinearClamp, pin.TexC, 0.0f);

    // return float4(f, 0.0f, 0.0f, 1.0f);  
    return float4(color, 1.0f);
}
