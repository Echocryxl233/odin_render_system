
#define MaxLights 1316

struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot light only
    float3 Direction;   // directional/spot light only
    float FalloffEnd;   // point/spot light only
    float3 Position;    // point light only
    float SpotPower;    // spot light only
};

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Shininess;
};

float3 SchlickFresnel(float3 fresnelR0, float3 normal, float3 light_direction) {
    float cos_incident_angle = saturate(dot(normal, light_direction));
    float f0 = 1.0f - cos_incident_angle;
    float3 reflect_percent = fresnelR0 + (1.0f - fresnelR0)* (f0*f0*f0*f0*f0);
    return reflect_percent;
}


float3 BlinnPhong(float3 light_color, float3 light_direction, float3 to_eye, float3 normal, Material mat) {
    const float m = mat.Shininess * 256.0f;

    float3 half_vector = normalize(to_eye + light_direction);

    float roughness_factor = (8.0f + m) * pow(max(0.0f, dot(normal, half_vector)), m) / 8.0f;
    float3 fresnel_factor = SchlickFresnel(mat.FresnelR0, normal, light_direction);
    
    float3 specular_albedo = roughness_factor * fresnel_factor;
    specular_albedo /=  (1.0f + specular_albedo);

    return (mat.DiffuseAlbedo.xyz + specular_albedo) * light_color;
}

float3 ComputeDirectLight(Light light, float3 to_eye, float3 normal, Material mat) {
    float3 light_direct = -light.Direction;
    float nDotl = max(0.0f, dot(normal, light_direct));
    float3 light_strength = light.Strength * nDotl;

    return BlinnPhong(light_strength, light_direct, to_eye, normal, mat);
}

float CalculateAttenuation(float d, float falloffStart, float falloffEnd) {

    return saturate((falloffEnd - d )/(falloffEnd - falloffStart));
}

float3 ComputePointLight(Light light, float3 pos, float3 to_eye, float3 normal, Material mat) {
    float d = distance(light.Position, pos);
    if (d > light.FalloffEnd)
        return float3(0.0f, 0.0f, 0.0f);

    float3 light_direction = light.Position - pos;
    light_direction /= d;

    float nDotl = max(0.0f, dot(light_direction, normal));
    float3 light_strength = light.Strength ;    //   * nDotl;

    float attenuation = CalculateAttenuation(d, light.FalloffStart, light.FalloffEnd);
    light_strength *= attenuation;

    // return BlinnPhong(light_strength, light_direction, to_eye, normal, mat);
    return light_strength * mat.DiffuseAlbedo;
    // return light_strength;
}



