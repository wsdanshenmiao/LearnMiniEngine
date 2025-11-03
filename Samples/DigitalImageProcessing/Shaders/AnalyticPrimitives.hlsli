#ifndef __ANALYTIC_PRIMITIVES_HLSLI__
#define __ANALYTIC_PRIMITIVES_HLSLI__

#include "RayTracingHLSLCompat.h"
#include "Common.hlsli"

using namespace RayTracing;


// 测试射线是否和球体相交
bool RaySphereIntersectionTest(
    in Ray ray,
    float2 interval,
    float3 center,
    float radius,
    out ProceduralPrimitiveAttributes attrs, 
    inout float time)
{
    float3 oc = center - ray.origin;
    float a = dot(ray.direction, ray.direction);
    float h = dot(ray.direction, oc);
    float c = dot(oc, oc) - radius * radius;

    float discriminant = h * h -  a * c;
    if (discriminant < 0) { // 没有根则不相交
        return false;
    }
    float sqrtD = sqrt(discriminant);
    float invA = 1.0f / a;
    float root = (h - sqrtD) * invA;  // 计算方程的根
    if (root <= interval.x || root >= interval.y) { //不再范围内
        root = (h + sqrtD) * invA;
        if (root <= interval.x || root >= interval.y) { //不再范围内
            return false;
        }
    }
    
    time = root;

    float3 pos = ray.origin + time * ray.direction; // 计算交点
    float3 normal = (pos - center) / radius;

    float invPI = 1.f / s_PI;
    float theta = acos(normal.y);
    float phi = atan2(normal.z, normal.x);
    attrs.uv = float2((phi + s_PI) * 0.5f * invPI, theta * invPI);
    attrs.frontFace = dot(ray.direction, normal) < 0;
    attrs.normal = attrs.frontFace ? normal : -normal;

    return true;
}

// 测试射线是否和四边形相交
bool RayQuadIntersectionTest(
    in Ray ray,
    float2 interval,
    float3 q,
    float3 u,
    float3 v,
    out ProceduralPrimitiveAttributes attrs, 
    inout float time)
{
    float3 normal = cross(u, v);
    float3 w = normal / dot(normal, normal);
    normal = normalize(normal);
    float d = dot(normal, q);

    // 判断射线是否与平面相交
    float nd = dot(normal, ray.direction);
    if(abs(nd) < 1e-4f)
        return false;

    // 计算交点
    time = (d - dot(normal, ray.origin)) / nd;
    if(!InRange(time, interval.x, interval.y))
        return false;
    
    float3 pos = ray.origin + time * ray.direction; // 计算交点

    float3 pq = pos - q;
    float alpha = dot(w, cross(pq, v));
    float beta = dot(w, cross(u, pq));

    if(!InRange(alpha, 0, 1) || !InRange(beta, 0, 1))
        return false;

    // 将法线变换到世界空间
    attrs.uv = float2(alpha, beta);
    attrs.frontFace = nd < 0;
    attrs.normal = attrs.frontFace ? normal : -normal;

    return true;
}

bool RayCubeIntersectionTest(
    in Ray ray,
    float2 interval,
    float3 boxMin,
    float3 boxMax,
    out ProceduralPrimitiveAttributes attrs, 
    inout float time)
{
    float3 origin = ray.origin;
    float3 dir = ray.direction;

    // [min, max]
    const float3 invDir = 1.0f / dir;
    float3 t0 = (boxMin - origin) * invDir;
    float3 t1 = (boxMax - origin) * invDir;

    for (uint i = 0; i < 3; ++i) {
        const float tmin = min(t0[i], t1[i]);
        const float tmax = max(t0[i], t1[i]);

        interval.x = max(interval.x, tmin);
        interval.y = min(interval.y, tmax);

        if (interval.x >= interval.y) 
            return false;
    }

    time = interval.x < 0 ? interval.y : interval.x;

    float3 pos = ray.origin + time * ray.direction; // 计算交点

    float3 boxVec = pos / max(max(abs(pos.x), abs(pos.y)), abs(pos.z));
    float3 normal = float3(
        abs(boxVec.x) > 0.9999 ? sign(boxVec.x) : 0,
        abs(boxVec.y) > 0.9999 ? sign(boxVec.y) : 0,
        abs(boxVec.z) > 0.9999 ? sign(boxVec.z) : 0
    );

    // 将法线变换到世界空间
    normal = normalize(normal);
    attrs.uv = (pos.xy - boxMin.xy) * 0.5f;
    attrs.frontFace = dot(ray.direction, normal) < 0;
    attrs.normal = attrs.frontFace ? normal : -normal;

    return true;
}

bool RayAnalyticPrimitiveIntersectionTest(
    in Ray ray,
    float2 interval,
    in AnalyticPrimitive::PrimitiveType primType, 
    out ProceduralPrimitiveAttributes attrs, 
    inout float time)
{
    bool hit = false;

    switch(primType){
    case AnalyticPrimitive::PrimitiveType::Sphere:{
        // 在局部坐标中的球心和半径
        const float3 center = float3(0,0,0);
        const float radius = 1;
        hit = RaySphereIntersectionTest(ray, interval, center, radius, attrs, time);
        break;
    }
    case AnalyticPrimitive::PrimitiveType::Quad:{
        // 单位四边形的参数
        const float3 q = float3(-1, -1, 0);
        const float3 u = float3(2, 0, 0);
        const float3 v = float3(0, 2, 0);
        hit = RayQuadIntersectionTest(ray, interval, q, u, v, attrs, time);
        break;
    }
    case AnalyticPrimitive::PrimitiveType::Cube:{
        float3 boxMin = float3(-1,-1,-1);
        float3 boxMax = float3(1,1,1);
        hit = RayCubeIntersectionTest(ray, interval, boxMin, boxMax, attrs, time);
        break;
    }
    default:
        hit = false;
        break;
    }

    if(hit){
        attrs.normal = mul(attrs.normal, (float3x3)transpose(WorldToObject4x3()));
        attrs.normal = normalize(attrs.normal);
    }

    return hit;
} 


#endif