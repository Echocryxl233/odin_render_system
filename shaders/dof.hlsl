static const int gMaxBlurRadius = 5;
cbuffer cbSettings : register(b0) {
    float FocalLength;
    float ZNear;
    float ZFar;
    float Aparture;
};


Texture2D BlurMap : register(t0);
Texture2D<float> DepthMap : register(t1);
Texture2D ColorMap : register(t2);

RWTexture2D<float4> gOutput : register(u0);

#define N 256
#define CacheSize (N + 2*gMaxBlurRadius)
groupshared float4 gCache[CacheSize];

[numthreads(1, N, 1)]
void DofCS(int3 groupThreadID : SV_GroupThreadID,
				int3 dispatchThreadID : SV_DispatchThreadID)
{
    float depth = DepthMap[dispatchThreadID.xy];
    float objectDistance = -ZFar * ZNear / (depth * (ZFar - ZNear) - ZFar);
    if (objectDistance > FocalLength) {
        gOutput[dispatchThreadID.xy] = BlurMap[dispatchThreadID.xy]; 
    }
    else {
        gOutput[dispatchThreadID.xy] = ColorMap[dispatchThreadID.xy];
    }
}