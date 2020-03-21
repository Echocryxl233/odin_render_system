Texture2D gInput            : register(t0);
RWTexture2D<float4> gOutput : register(u0);

#define N 256
#define CacheSize (N)
groupshared float4 gCache[CacheSize];

[numthreads(1, N, 1)]
void Bloom(int3 groupThreadID : SV_GroupThreadID,
        int3 dispatchThreadID : SV_DispatchThreadID) {

    const float gamma = 2.2f;
    float3 hdr =  gInput[dispatchThreadID.xy];
    float a = gInput[dispatchThreadID.xy].a;

    float exposure = 1.0f;

    float brightness = dot(hdr,  float3(0.2126f, 0.7152f, 0.0722f));

    if (brightness > 1.0f)
        gOutput[dispatchThreadID.xy] = float4(hdr, a);
    else 
       gOutput[dispatchThreadID.xy] = float4(0.0f, 0.0f, 0.0f, a);

}