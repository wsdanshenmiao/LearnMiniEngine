#ifndef __LIGHT_HLSLI__
#define __LIGHT_HLSLI__

#include "Surface.hlsli"
#include "BRDF.hlsli"
#include "Common.hlsli"

struct Light
{
    float3 color;
    float3 direction;
    float attenuation;
};

ConstantBuffer<LightData> gLightData : register(b0, space1);

StructuredBuffer<DirectionalLightData> gDirLightData : register(t0, space1);


// 追踪阴影光线
bool TraceShadowRay(RayDesc ray, uint depth)
{
    [branch]
    if(depth >= MAX_TRACE_RECURSION_DEPTH) {
        return false;
    }

    RayTracing::ShadowRayPayload payload;
    payload.visible = false;
    TraceRay(gScene, 
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES
        | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
        | RAY_FLAG_FORCE_OPAQUE             // ~skip any hit shaders
        | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, // ~skip closest hit shaders,
        RayTracing::TraceRayParameters::InstanceMark, 
        RayTracing::TraceRayParameters::HitGroup::Offset[RayTracing::RayType::Shadow], 
        RayTracing::TraceRayParameters::HitGroup::GeometryStride, 
        RayTracing::TraceRayParameters::MissShader::Offset[RayTracing::RayType::Shadow], 
        ray, 
        payload);

    return payload.visible;
}

uint GetDirectionalLightCount()
{
    return gLightData.dirLightCount;
}


Light GetDirectionalLight(uint index, Surface surface)
{
    DirectionalLightData lightData = gDirLightData[index];
    // 计算阴影
    RayDesc ray;
    ray.Origin = surface.position;
    ray.Direction = lightData.direction.xyz;
    ray.TMin = 0.001f;
    ray.TMax = 10000.0f;
    bool visible = TraceShadowRay(ray, surface.recursionDepth);
    //bool visible = true;
    Light light;
    light.color = lightData.color.rgb;
    light.direction = normalize(lightData.direction.xyz);
    light.attenuation = visible ? 1.0f : 0.0f;
    return light;
}

float3 ShadeLighting(Surface surface, Light light)
{
    float3 halfDir = normalize(light.direction + surface.viewDir);
    float NoV = saturate(dot(surface.normal, surface.viewDir));
    float NoL = saturate(dot(surface.normal, light.direction));
    float NoH = saturate(dot(surface.normal, halfDir));
    float LoH = saturate(dot(light.direction, halfDir));    // 与 VoH 相同

    float3 f0 = lerp(s_DielectricSpecular, surface.color, surface.metallic);
    float3 specular = SpecularBRDF(NoV, NoL, NoH, LoH, f0, surface.roughness);
    float3 diffuse = DiffuseBurley(NoV, NoL, LoH, surface.roughness);
    float3 diffuseCol = surface.color * (1 - surface.metallic);
    diffuse *= diffuseCol;

    float3 radians = NoL * light.color * (diffuse + specular);
    return radians * light.attenuation;
}

float3 ShadeLighting(Surface surface)
{
    float3 color = 0;
    for(uint i = 0; i < GetDirectionalLightCount(); i++) {
        Light dirLight = GetDirectionalLight(i, surface);
        color += ShadeLighting(surface, dirLight);
    }
    return color;
}

#endif