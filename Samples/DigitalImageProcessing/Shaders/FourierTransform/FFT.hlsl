#ifndef __FFT_HLSL__
#define __FFT_HLSL__

#include "../Complex.hlsli"

Texture2D<float4> gInputTex : register(t0);
StructuredBuffer<uint> gReverseIndices : register(t1);


#endif