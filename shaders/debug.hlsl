Texture2D gSsaoMap  : register(t0, space1);
SamplerState gsamLinearWrap  : register(s0);

struct VertexIn
{
  float3 PosL : POSITION;
  float3 Normal : NORMAL;
  float2 TexC : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
    // Already in homogeneous clip space.
    vout.PosH = float4(vin.PosL, 1.0f);
	vout.TexC = vin.TexC;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
  //  return float4(gShadowMap.Sample(gsamLinearWrap, pin.TexC).rrr, 1.0f);    
  return float4(gSsaoMap.Sample(gsamLinearWrap, pin.TexC).rrr, 1.0f);    
	//  return float4(gSsaoMap.Sample(gsamLinearWrap, pin.TexC).rgb, 1.0f);   
	// return float4(1.0f, 1.0f, 0.0f, 1.0f);
}
