Texture2D gInput            : register(t0);
RWTexture2D<float4> gOutput : register(u0);

#define N 256
// #define CacheSize (N)
// groupshared float4 gCache[N];

[numthreads(1, N, 1)]
void HDRCS(int3 groupThreadID : SV_GroupThreadID,
        int3 dispatchThreadID : SV_DispatchThreadID) {

    const float gamma = 2.2f;
    float3 hdr_color =  gInput[dispatchThreadID.xy];
    float3 result = hdr / (hdr + (float3)1.0f);
    result = pow(result,  (float3)(1.0f / gamma));

    float exposure = 1.0f;

    gOutput[dispatchThreadID.xy] = result
}