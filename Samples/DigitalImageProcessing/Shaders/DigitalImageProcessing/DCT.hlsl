#include "../Complex.hlsli"

#ifndef THREAD_SIZE
#define THREAD_SIZE 16
#endif

#define SQUARE_THREAD_SIZE (THREAD_SIZE * THREAD_SIZE)

static const float sPI = 3.14159265359f;

Texture2D<float2> gInputTex : register(t0);
RWTexture2D<float2> gOutputTex : register(u0);

[numthreads(SQUARE_THREAD_SIZE, 1, 1)]
void OriginPointDCTCS(
    uint3 groupThreadID : SV_GroupThreadID,
    uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    gOutputTex.GetDimensions(width, height);
    if(dispatchThreadID.x >= width || dispatchThreadID.y >= height)
        return;
    
    uint len = height;
    uint start = groupThreadID.x * THREAD_SIZE;
    float sum = 0;
#if defined(IS_VERTICAL)
    len = width;
    if(!all(dispatchThreadID)){
        sum = gOutputTex[uint2(0, 0)].x;
    }
#endif
    for(uint i = 0; i < len; ++i){
        uint2 index = uint2(dispatchThreadID.x, i);
#if defined(IS_VERTICAL)
        index = uint2(i, dispatchThreadID.x);
#endif
        sum += gInputTex[index].x;
    }

    gOutputTex[dispatchThreadID.xy] = float2(sum * rsqrt(len), 0.0f);
}


[numthreads(THREAD_SIZE, THREAD_SIZE, 1)]
void DCTCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    gOutputTex.GetDimensions(width, height);
    if(dispatchThreadID.x >= width || 
        dispatchThreadID.y >= height ||
        (dispatchThreadID.x * dispatchThreadID.y) == 0)
        return;

    Complex inputVal = Complex(gInputTex[dispatchThreadID.xy]);
    float angle = sPI * -0.5f * ((float(dispatchThreadID.x) / width + float(dispatchThreadID.y) / height));
#if defined(IS_INVERSE)
    angle *= -2;
#endif
    inputVal = cmul(inputVal, cexp(angle));

    float factorX = sqrt(2.0f / width);
    float factorY = sqrt(2.0f / height);

    float2 outputVal = float2(inputVal.value.x * factorX * factorY, 0.0f);
#if defined(IS_INVERSE)
    outputVal.x += gInputTex[uint2(0, 0)].x * (rsqrt(width) - rsqrt(width * 0.5f)) * (rsqrt(height) - rsqrt(height * 0.5f));
#endif
    gOutputTex[dispatchThreadID.xy] = outputVal;
}