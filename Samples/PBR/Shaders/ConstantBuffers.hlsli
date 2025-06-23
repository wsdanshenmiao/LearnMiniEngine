#ifndef __CONSTANTBUFFERS_HLSLI__
#define __CONSTANTBUFFERS_HLSLI__

struct MeshConstants
{
    float4x4 World;
    float4x4 WorldIT;
};

struct MaterialConstants
{
    float4 BaseColor;
    float3 EmissiveColor;
    float NormalTexScale;
    float MetallicFactor;
    float RoughnessFactor;
};
struct PassConstants
{
    float4x4 View;
    float4x4 ViewInv;
    float4x4 Proj;
    float4x4 ProjInv;
    float4x4 ShadowTrans;
    float3 CameraPos;
    float TotalTime;
    float DeltaTime;
};


#endif