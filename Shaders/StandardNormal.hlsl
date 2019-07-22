#include "Common.hlsl"

struct VertexIn
{
	float3 PosL  : POSITION;
  float3 Normal : NORMAL; // normal model
  float2 TexCoord : TEXCOORD;
  float3 TangentU : TANGENT;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
  float3 PosW  : POSITION;
  float3 Normal : NORMAL; //  normal world
  float2 TexCoord : TEXCOORD;
  float3 TangentW : TANGENT;

  nointerpolation uint MatIndex  : MATINDEX;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout = (VertexOut)0.0f;
    InstanceData instData = gInstanceData[instanceID];

    float4x4 world = instData.World;
	  float4x4 texTransform = instData.TexTransform;
	  uint matIndex = instData.MaterialIndex;
    vout.MatIndex = matIndex;

    MaterialData matData = gMaterialData[matIndex];
    vout.TangentW = mul(vin.TangentU, (float3x3)world);

	  // Transform to homogeneous clip space.
    float4 posW = mul(float4(vin.PosL, 1.0f), world);
    vout.PosW = posW.xyz;
    vout.PosH = mul(posW, gViewProj);
	
	  // Just pass vertex color into the pixel shader.
    //  vout.Color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    vout.Normal = mul(vin.Normal, (float3x3)world);
    float4 texCoord = mul(float4(vin.TexCoord, 0.0f, 1.0f), texTransform);
    vout.TexCoord = mul(texCoord, matData.MatTransform).xy;

    return vout;
}

float4 PS(VertexOut pin) : SV_TARGET
{
    //  MaterialData matData = gMaterialData[MaterialIndex];
    MaterialData matData = gMaterialData[pin.MatIndex];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float3 FresnelR0 = matData.FresnelR0;
    float  Roughness = matData.Roughness;
  
    uint deffuseMapIndex = matData.DiffuseMapIndex;
    diffuseAlbedo = gTextureMaps[deffuseMapIndex].Sample(gsamLinearWrap, pin.TexCoord) * diffuseAlbedo;

    uint normalMapIndex = matData.NormalMapIndex;
    pin.Normal = normalize(pin.Normal);

    float4 normalMapSample = gTextureMaps[normalMapIndex].Sample(gsamAnisotropicWrap, pin.TexCoord);
    float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.Normal, pin.TangentW);
    float3 toEyeW = normalize(gEyePosW - pin.PosW);
    float4 ambient = gAmbientLight*diffuseAlbedo;

    const float shininess = 1.0f - Roughness;
    Material mat = { diffuseAlbedo, FresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    //float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
    //    pin.Normal, toEyeW, shadowFactor);

    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
      bumpedNormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    // Add in specular reflections.
    float3 r = reflect(-toEyeW, bumpedNormalW);
    float4 reflectionColor = gCubeMap.Sample(gsamLinearWrap, r);
    float3 fresnelFactor = SchlickFresnel(FresnelR0, bumpedNormalW, r);
    litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;

    // Common convention to take alpha from diffuse material.
    litColor.a = diffuseAlbedo.a;

    return litColor;

}


