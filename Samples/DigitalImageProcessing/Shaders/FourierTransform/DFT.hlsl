#include "../Complex.hlsli"

#ifndef THREAD_SIZE
#define THREAD_SIZE 256
#endif

static const float sPI = 3.14159265359f;
static const float sTwoPI = 6.28318530718f;

Texture2D gDFTInputTex : register(t0);
RWTexture2D<float2> gDFTOutputTex : register(u0);

#if defined(ENABLE_DEBUG_OUTPUT)
RWTexture2D<float4> gDebugOutputTex : register(u1);
#endif


groupshared Complex DataCache[THREAD_SIZE];

Complex CalculateDFT(Complex fxy, float u, float x, float factor)
{
    // 计算幅角
    float angle = (u * x) * factor;
    angle = fmod(angle, sTwoPI);
    Complex expTerm = cexp(angle);
    return cmul(fxy, expTerm);
}

Complex GetInputData(uint2 index)
{
    Complex fxy = Complex(gDFTInputTex[index].xy);
    return fxy;
}

[numthreads(THREAD_SIZE, 1, 1)]
void LuminanceDFTCS(
    uint3 groupID : SV_GroupID, 
    uint3 groupThreadID : SV_GroupThreadID, 
    uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    gDFTInputTex.GetDimensions(width, height);
    
    uint u = dispatchThreadID.x;
    uint v = dispatchThreadID.y;

    uint len = width;
    uint selectedDim = dispatchThreadID.x;  // 垂直的时候是 y 轴的坐标 
    uint start = groupID.x * THREAD_SIZE;

#if defined(IS_VERTICAL)
    // 交换 u,v 以及长度
    u = dispatchThreadID.y;
    v = dispatchThreadID.x;
    len = height;
#endif

    if (u >= width || v >= height)
        return;

    uint2 threadIndex = uint2(u, v);

    // 储存数据到组内共享内存
    DataCache[groupThreadID.x] = Complex(gDFTInputTex[threadIndex].xy);
    GroupMemoryBarrierWithGroupSync();
    
    Complex sum = Complex(float2(0.0f, 0.0f));
    float factor = -sTwoPI / float(len);
#if defined(IS_INVERSE_DFT)
    factor *= -1;
#endif

    // 先计算共享内存中的 DFT
    [unroll(THREAD_SIZE)]
    for (uint i = 0; i < THREAD_SIZE && (start + i) < len; ++i) {
        sum = cadd(sum, CalculateDFT(DataCache[i], selectedDim, i + start, factor));
    }

    [loop]
    for (uint i = 0; i < len; ++i) {
        if(i == start){
            i += THREAD_SIZE;
            if(i >= len)
                break;
        }
#if defined(IS_VERTICAL)
        uint2 index = uint2(u, i);
#else
        uint2 index = uint2(i, v);
#endif
        Complex fxy = Complex(gDFTInputTex[index].xy);
        sum = cadd(sum, CalculateDFT(fxy, selectedDim, i, factor));
    }
#if defined(IS_INVERSE_DFT)
    sum.value /= float(len);
#endif

    gDFTOutputTex[threadIndex] = sum.value;

#if defined(ENABLE_DEBUG_OUTPUT)
    gDebugOutputTex[threadIndex] = float4(length(sum.value).rrr / (width + height), 1);
#endif
}


