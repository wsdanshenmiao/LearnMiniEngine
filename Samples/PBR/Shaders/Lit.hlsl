#include "Common.hlsli"
#include "ConstantBuffers.hlsli"

ConstantBuffer<MeshConstants> _MeshConstants : register(b0);
ConstantBuffer<MaterialConstants> _MaterialConstants : register(b1);
ConstantBuffer<PassConstants> _PassConstants : register(b2);

// PBR相关纹理
Texture2D<float4> _BaseColorTex : register(t0);
Texture2D<float4> _DiffuseRoughnessTex : register(t1);
Texture2D<float> _MetalnessTex : register(t2);
Texture2D<float> _OcclusionTex : register(t3);
Texture2D<float3> _EmissiveTex : register(t4);
Texture2D<float3> _NormalTex : register(t5);

// 从第十个纹理开始
Texture2D<float> _ShadowTex : register(t10);

struct Attributes
{
    float3 posOS : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
#if defined(USE_TANGENT)
    float4 tangent : TANGENT;
#endif
};

struct Varyings
{
    float4 posCS : SV_POSITION;
    float3 normal : NORMAL;
#if defined(USE_TANGENT)
    float4 tangent : TANGENT;
#endif
    float2 uv : TEXCOORD0;
    float3 posWS : TEXCOORD1;
    float3 posShadow : TEXCOORD2;
};



Varyings LitPassVS(Attributes i)
{
    Varyings o;

    float4x4 viewProj = mul(_PassConstants.View, _PassConstants.Proj);

    float4 posWS = mul(float4(i.posOS, 1), _MeshConstants.World);
    float3 normal = mul(i.normal, (float3x3)_MeshConstants.WorldIT);
    o.posWS = posWS.xyz;
    o.posCS = mul(posWS, viewProj);
    o.uv = i.uv;
    o.normal = normalize(normal);
#if defined(USE_TANGENT)
    o.tangent.xyz = mul(i.tangent.xyz, (float3x3)_MeshConstants.WorldIT).xyz;
#endif
    o.posShadow = mul(float4(o.posWS, 1), _PassConstants.ShadowTrans).xyz;

    return o;
}



float4 LitPassPS(Varyings i) : SV_TARGET0
{
    float4 baseCol = _BaseColorTex.Sample(defaultSampler, i.uv);
    float4 diffuseRoughness = _DiffuseRoughnessTex.Sample(defaultSampler, i.uv);
    float metalness = _MetalnessTex.Sample(defaultSampler, i.uv);
    float occlusion = _OcclusionTex.Sample(defaultSampler, i.uv);
    float3 emissive = _EmissiveTex.Sample(defaultSampler, i.uv);
    float3 normal = _NormalTex.Sample(defaultSampler, i.uv);

    baseCol.rgb += emissive;
    baseCol.rgb *= occlusion;
    baseCol.rgb *= metalness;
    baseCol.rgb *= diffuseRoughness.rgb;
    normal *= i.normal;

    return float4(dot(float3(0,1,0), normalize(normal)) * baseCol.rgb, baseCol.a);
}