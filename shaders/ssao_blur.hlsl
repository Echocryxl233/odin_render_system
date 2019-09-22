#include "ssao_common.hlsl"

struct VertexOut {
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
};

static const int gBlurRadius = 5;

VertexOut VS(uint vid : SV_VERTEXID) {
    VertexOut vout = (VertexOut)0.0f;
    vout.TexC = gTexCoods[vid];
    vout.PosH = float4(2.0f * vout.TexC.x - 1.0f, 1.0f - 2.0f * vout.TexC.y, 0.0f, 1.0f);   //  [0, 1] -> [-1, 1]
    return vout;
}

float4 PS(VertexOut pin) : SV_TARGET
{
    float blur_weight[12] = {
        gBlurWeights[0].x, gBlurWeights[0].y, gBlurWeights[0].z,  gBlurWeights[0].w,
        gBlurWeights[1].x, gBlurWeights[1].y, gBlurWeights[1].z,  gBlurWeights[1].w,
        gBlurWeights[2].x, gBlurWeights[2].y, gBlurWeights[2].z,  gBlurWeights[2].w
    };

    float2 tex_offset;
    if (gHorizontalBlur) {
        tex_offset = float2(gInvRenderTargetSize.x, 0.0f);
    }
    else {
        tex_offset = float2(0.0f, gInvRenderTargetSize.y);
    }

    float4 color = blur_weight[gBlurRadius] * gRandomMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f);
    float total_weight = blur_weight[gBlurRadius];

    float3 center_normal = gNormalMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).xyz;
    float center_depth = gDepthMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).r;
    center_depth = NdcDepthToViewDepth(center_depth);

    for (float i = -gBlurRadius; i<=gBlurRadius; ++i) {
        if (i == 0)
            continue;

        float2 tex = pin.TexC + i*tex_offset;

        float3 neighbor_normal = gNormalMap.SampleLevel(gsamPointClamp, tex, 0.0f).xyz;
        float  neighbor_depth = gDepthMap.SampleLevel(gsamPointClamp, tex, 0.0f).r;

        if (dot(neighbor_normal, center_normal) <= 0.8f &&
            abs(neighbor_depth - center_depth) <= 0.2f) 
        {
            float weight = blur_weight[i + gBlurRadius];

            color += weight * gRandomMap.SampleLevel(gsamPointClamp, tex, 0.0f);
            total_weight += weight;
        }
    }

    return color / total_weight;
    // return color;
    // float3 input = gRandomMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).xyz;
    // return float4(input, 1.0f);
}