#include "ssao_common.hlsl"


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
    float4 ph = mul(vout.PosH, gInvProj);
    vout.PosV = ph.xyz / ph.w;
    return vout;
}

float OcclusionFunction(float dist_z) {
    float occlusion = 0.0f;
    if (dist_z > gSurfaceEpsilon) {
        float length = gOcclusionFadeEnd - gOcclusionFadeStart;
        occlusion = saturate((gOcclusionFadeEnd - dist_z) / length);
    }
    return occlusion;
}

float4 PS(VertexOut pin) : SV_TARGET
{
    float3 n = normalize(gNormalMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).xyz);
    float pz = gDepthMap.SampleLevel(gsamDepthMap, pin.TexC, 0.0f).r;
    pz = NdcDepthToViewDepth(pz);

    float3 p = (pz / pin.PosV.z) * pin.PosV;

    float3 ramdon_vec = 2.0f * gRandomMap.SampleLevel(gsamPointClamp, 4.0f*pin.TexC, 0.0f).xyz - 1.0f;

    float occlusion_sum = 0.0f;
    for (int i=0; i<gSampleCount; ++i) {
        // float3 offset = reflect(gOffsetVectors[i], ramdon_vec);
        // float flip = sign(dot(offset, n));
        // float3 q = p + flip*offset*gOcclusionRadius;

        float3 q = p + gOffsetVectors[i];

        float4 projQ = mul(float4(q, 1.0f), gProjTex);      // projQ is the position in texcoord after projection
        projQ /= projQ.w;
        
        float rz = gDepthMap.SampleLevel(gsamDepthMap, projQ.xy, 0.0f).r;
        rz = NdcDepthToViewDepth(rz);
        
        float3 r = (rz / q.z) * q;

        float dist_z = p.z - r.z;
		float dp = max(dot(n, normalize(r - p)), 0.0f);

        float occlusion = dp*OcclusionFunction(dist_z);
        occlusion_sum += occlusion;
        
        // float dp = max(0.0f, dot(n, q));
        // float dist_z = pz - rz;
        // occlusion_sum += dp * OcclusionFunction(dist_z);
    }

    occlusion_sum /= gSampleCount;

    float access = 1.0f - occlusion_sum;

    return saturate(pow(access, 6.0f));
}
