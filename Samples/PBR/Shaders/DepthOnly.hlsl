#include "Common.hlsli"
#include "ConstantBuffers.hlsli"


ConstantBuffer<MeshConstants> _MeshConstants : register(b0);
ConstantBuffer<MaterialConstants> _MaterialConstants : register(b1);
ConstantBuffer<PassConstants> _PassConstants : register(b2);

Texture2D<float4> _DiffuseTex : register(t0);

struct Attributes
{
	float3 posOS : POSITION;
	float2 uv : TEXCOORD0;
};

struct Varyings
{
	float4 posCS : SV_POSITION;
	float2 uv : TEXCOORD0;
};

Varyings DepthOnlyPassVS(Attributes i)
{
	Varyings o;
	float4x4 viewProj = mul(_PassConstants.View, _PassConstants.Proj);
	float3 posWS = mul(float4(i.posOS, 1), _MeshConstants.World).xyz;
	o.posCS = mul(float4(posWS, 1), viewProj);
	o.uv = i.uv;
	return o;
}

void DepthOnlyPassPS(Varyings i)
{
	float4 col = _DiffuseTex.Sample(defaultSampler, i.uv);
	float alpha = col.a * _MaterialConstants.BaseColor.a;
#ifdef ALPHA_TEST
	clip(alpha - 0.1f);
#endif
}

float4 DepthOnlyDebugPassPS(Attributes i)
{
	float depth = _DiffuseTex.Sample(defaultSampler, i.uv).r;
	return float4(depth.rrr, 1);
}