Texture2D gHDRTexture            : register(t0);
Texture2D gBloomTexture            : register(t1);
RWTexture2D<float4> gOutput : register(u0);

#define N 256
#define CacheSize (N)
groupshared float4 gCache[CacheSize];

float3 ACESToneMapping(float3 color, float adapted_lum) 
{ 	
    const float A = 2.51f; 	
    const float B = 0.03f; 	
    const float C = 2.43f; 	
    const float D = 0.59f; 	
    const float E = 0.14f;  	
    return (color * (A * color + B)) / (color * (C * color + D) + E); 
}

[numthreads(1, N, 1)]
void BloomComposite(int3 groupThreadID : SV_GroupThreadID,
        int3 dispatchThreadID : SV_DispatchThreadID) {

    const float gamma = 2.2f;
    float3 hdr =  gHDRTexture[dispatchThreadID.xy].xyz;
    float  a = gHDRTexture[dispatchThreadID.xy].a;
    float3 bloom =  gBloomTexture[dispatchThreadID.xy];

    float exposure = 1.0f;


    // float brightness = dot((float3)gInput[dispatchThreadID.xy],  float3(0.2126f, 0.7152f, 0.0722f));
    // //   gOutput[dispatchThreadID.xy] = float4(1.0f, 0.0f, 1.0f, 1.0f);
    // // float3 bloom = float3(0.0f);

    // if (brightness > 0.95f)
    //     gOutput[dispatchThreadID.xy] = gInput[dispatchThreadID.xy];
    // else 
    //    gOutput[dispatchThreadID.xy] = float4(0.0f, 0.0f, 0.0f, 1.0f);
    // float3 bloom = gOutput[dispatchThreadID.xy];

    hdr += bloom;

    // //  tone mapping
    // float3 result = hdr / (hdr + (float3)1.0f);
    float3 result = ACESToneMapping(hdr, 0.0f);

    // //  gamma correct
    result = pow(result,  (float3)(gamma));

    gOutput[dispatchThreadID.xy] = float4(result, 1.0f);
}