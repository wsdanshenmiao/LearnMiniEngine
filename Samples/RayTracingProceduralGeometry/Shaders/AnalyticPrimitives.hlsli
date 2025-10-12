#ifndef __ANALYTIC_PRIMITIVES_HLSLI__
#define __ANALYTIC_PRIMITIVES_HLSLI__

#include "RayTracingHLSLCompat.h"
#include "Common.hlsli"

using namespace RayTracing;

bool RayCubeIntersectionTest(in Ray ray, float3 boxMin, float3 boxMax, inout float time)
{
    float3 origin = ray.origin;
    float3 dir = ray.direction;

    // [min, max]
    const float3 invDir = 1.0f / dir;
    float3 t0 = (boxMin - origin) * invDir;
    float3 t1 = (boxMax - origin) * invDir;
    
    float2 interval = float2(RayTMin(), RayTCurrent());
    for (uint i = 0; i < 3; ++i) {
        const float tmin = (t0[i] < t1[i]) ? t0[i] : t1[i];
        const float tmax = (t0[i] < t1[i]) ? t1[i] : t0[i];

        if (tmin > interval.x) interval.x = tmin;
        if (tmax < interval.y) interval.y = tmax;

        if (interval.x >= interval.y) return false;
    }

    time = interval.x;
    return true;
}

// 测试射线是否和球体相交
bool RaySphereIntersectionTest(
    in Ray ray, 
    out ProceduralPrimitiveAttributes attrs, 
    inout float time)
{
    // 在局部坐标中的球心和半径
    const float3 center = float3(0,0,0);
    const float radius = 1;

    float3 oc = center - ray.origin;
    float a = dot(ray.direction, ray.direction);
    float h = dot(ray.direction, oc);
    float c = dot(oc, oc) - radius * radius;
    float invA = 1.0f / a;

    float discriminant = h * h -  a * c;
    if (discriminant < 0) { // 没有根则不相交
        return false;
    }
    float sqrtD = sqrt(discriminant);
    float root = (h - sqrtD) * invA;  // 计算方程的根
    if (root < RayTMin() || root > RayTCurrent()) { //不再范围内
        root = (h + sqrtD) * invA;
        if (root < RayTMin() || root > RayTCurrent()) { //不再范围内
            return false;
        }
    }

    float3 pos = ray.origin + root * ray.direction; // 计算交点
    float3 posWS = mul(float4(pos,1), ObjectToWorld4x3()).xyz; // 转到世界空间
    float3 centerWS = mul(float4(center,1), ObjectToWorld4x3()).xyz;
    float3 normal = normalize(posWS - centerWS);

    float invPI = 1.f / s_PI;
    float theta = acos(normal.y);
    float phi = atan2(normal.z, normal.x);
    attrs.uv = float2((phi + s_PI) * 0.5f * invPI, theta * invPI);
    attrs.frontFace = dot(ray.direction, normal) < 0;
    attrs.normal = attrs.frontFace ? normal : -normal;
    
    time = root;

    return true;
}

// 测试射线是否和四边形相交
bool RayQuadIntersectionTest(
    in Ray ray, 
    out ProceduralPrimitiveAttributes attrs, 
    inout float time)
{
    // 单位四边形的参数
    const float3 q = float3(-1, -1, 0);
    const float3 u = float3(2, 0, 0);
    const float3 v = float3(0, 2, 0);

    float3 w = float3(0, 0, 0.25f);
    float3 normal = float3(0, 0, 1);

    // 判断射线是否与平面相交
    float nd = dot(normal, ray.direction);
    if(abs(nd) < 1e-4f)
        return false;

    // 计算交点
    time = -dot(normal, ray.origin) / nd;
    float3 pos = ray.origin + time * ray.direction; // 计算交点

    float3 pq = pos - q;
    float alpha = dot(w, cross(pq, v));
    float beta = dot(w, cross(u, pq));

    if(!InRange(alpha, 0, 1) || !InRange(beta, 0, 1))
        return false;

    // 将法线变换到世界空间
    normal = mul(normal, (float3x3)transpose(WorldToObject4x3()));
    attrs.uv = float2(alpha, beta);
    attrs.frontFace = nd < 0;
    attrs.normal = attrs.frontFace ? normal : -normal;

    return true;
}

bool RayCubeIntersectionTest(
    in Ray ray, 
    out ProceduralPrimitiveAttributes attrs, 
    inout float time)
{
    float3 boxMin = float3(-1,-1,-1);
    float3 boxMax = float3(1,1,1);

    if(!RayCubeIntersectionTest(ray, boxMin, boxMax, time)){
        return false;
    }

    float3 pos = ray.origin + time * ray.direction; // 计算交点

    float3 boxVec = pos / max(max(abs(pos.x), abs(pos.y)), abs(pos.z));
    float3 normal = float3(
        abs(boxVec.x) > 0.9999 ? sign(boxVec.x) : 0,
        abs(boxVec.y) > 0.9999 ? sign(boxVec.y) : 0,
        abs(boxVec.z) > 0.9999 ? sign(boxVec.z) : 0
    );

    // 将法线变换到世界空间
    normal = mul(normal, (float3x3)transpose(WorldToObject4x3()));
    attrs.uv = (pos.xy - boxMin.xy) * 0.5f;
    attrs.frontFace = dot(ray.direction, normal) < 0;
    attrs.normal = attrs.frontFace ? normal : -normal;

    return true;
}

bool RayAnalyticPrimitiveIntersectionTest(
    in Ray ray, 
    in AnalyticPrimitive::PrimitiveType primType, 
    out ProceduralPrimitiveAttributes attrs, 
    inout float time)
{
    switch(primType){
    case AnalyticPrimitive::PrimitiveType::Sphere:
        return RaySphereIntersectionTest(ray, attrs, time);
    case AnalyticPrimitive::PrimitiveType::Quad:
        return RayQuadIntersectionTest(ray, attrs, time);
    case AnalyticPrimitive::PrimitiveType::Cube:
        return RayCubeIntersectionTest(ray, attrs, time);
    default:
        return false;
    }
} 


#endif