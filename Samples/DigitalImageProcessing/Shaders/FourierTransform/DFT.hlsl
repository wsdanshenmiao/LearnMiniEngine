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
    float angle = -(u * x) * factor;
    angle = fmod(angle, sTwoPI);
    Complex expTerm = cexp(angle);
    return cmul(fxy, expTerm);
}

Complex GetInputData(uint2 index)
{
#if defined(IS_VERTIC_DFT)
    Complex fxy = Complex(gDFTInputTex[index].xy);
#else
    float4 data = gDFTInputTex[index];
    float luminance = dot(data.rgb, float3(0.299f, 0.587f, 0.114f));
    Complex fxy = Complex(float2(luminance, 0.0f));
#endif
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

#if defined(IS_VERTIC_DFT)
    // 交换 u,v 以及长度
    u = dispatchThreadID.y;
    v = dispatchThreadID.x;
    len = height;
#endif

    if (u >= width || v >= height)
        return;

    uint2 threadIndex = uint2(u, v);

    // 储存数据到组内共享内存
    DataCache[groupThreadID.x] = GetInputData(threadIndex);
    GroupMemoryBarrierWithGroupSync();
    
    Complex sum = Complex(float2(0.0f, 0.0f));
    float factor = sTwoPI / float(len);

    // 先计算共享内存中的 DFT
    for (uint i = 0; i < THREAD_SIZE && (start + i) < len; ++i) {
        sum = cadd(sum, CalculateDFT(DataCache[i], selectedDim, i + start, factor));
    }

    [loop]
    for (uint i = 0; i < len; ++i) {
        if(i == start)
            i += THREAD_SIZE;
#if defined(IS_VERTIC_DFT)
        uint2 index = uint2(u, i);
#else
        uint2 index = uint2(i, v);
#endif
        Complex fxy = GetInputData(index);
        sum = cadd(sum, CalculateDFT(fxy, selectedDim, i, factor));
    }

    gDFTOutputTex[threadIndex] = sum.value;
#if defined(ENABLE_DEBUG_OUTPUT)
    gDebugOutputTex[threadIndex] = float4(length(sum.value).rrr / (width + height), 1);
#endif
}





Texture2D<float2> gIDFTInputTex : register(t0);
RWTexture2D<float2> gIDFTHorizOutputTex : register(u0);
RWTexture2D<float4> gIDFTVerticOutputTex : register(u0);


Complex CalculateIDFT(Complex fxy, float u, float x, float factor)
{
    // 计算幅角
    float angle = (u * x) * factor;
    angle = fmod(angle, sTwoPI);
    Complex expTerm = cexp(angle);
    return cmul(fxy, expTerm);
}

[numthreads(THREAD_SIZE, 1, 1)]
void HorizLuminanceIDFTCS(
    uint3 groupID : SV_GroupID, 
    uint3 groupThreadID : SV_GroupThreadID, 
    uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    gIDFTInputTex.GetDimensions(width, height);

    uint u = dispatchThreadID.x;
    uint v = dispatchThreadID.y;
    
    if (u >= width || v >= height)
        return;

    Complex sum = (Complex)0;

    float factor = sTwoPI / float(width);

    // 储存数据到组内共享内存
    DataCache[groupThreadID.x] = Complex(gIDFTInputTex[uint2(u, v)].xy);
    GroupMemoryBarrierWithGroupSync();

    uint startX = groupID.x * THREAD_SIZE;
    // 先计算共享内存中的 DFT
    for (uint i = 0; i < THREAD_SIZE && (startX + i) < width; ++i) {
        sum = cadd(sum, CalculateIDFT(DataCache[i], u, i + startX, factor));
    }

    [loop]
    for (uint x = 0; x < startX; ++x) {
        Complex fxy = Complex(gIDFTInputTex[uint2(x, v)].xy);
        sum = cadd(sum, CalculateIDFT(fxy, u, x, factor));
    }
    [loop]
    for(uint x = startX + THREAD_SIZE; x < width; ++x) {
        Complex fxy = Complex(gIDFTInputTex[uint2(x, v)].xy);
        sum = cadd(sum, CalculateIDFT(fxy, u, x, factor));
    }
    sum.value /= float(width);

    gIDFTHorizOutputTex[dispatchThreadID.xy] = sum.value;
}

[numthreads(1, THREAD_SIZE, 1)]
void VerticLuminanceIDFTCS(
    uint3 groupID : SV_GroupID, 
    uint3 groupThreadID : SV_GroupThreadID, 
    uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    gIDFTInputTex.GetDimensions(width, height);

    uint u = dispatchThreadID.x;
    uint v = dispatchThreadID.y;
    
    if (u >= width || v >= height)
        return;
    
    // 储存数据到组内共享内存
    DataCache[groupThreadID.y] = Complex(gIDFTInputTex[uint2(u, v)].xy);
    GroupMemoryBarrierWithGroupSync();
    
    Complex sum = (Complex)0;

    float factor = sTwoPI / float(height);

    uint startY = groupID.y * THREAD_SIZE;
    // 先计算共享内存中的 DFT
    for (uint i = 0; i < THREAD_SIZE && (startY + i) < height; ++i) {
        sum = cadd(sum, CalculateIDFT(DataCache[i], v, i + startY, factor));
    }

    [loop]
    for (uint y = 0; y < startY; ++y) {
        Complex fxy = Complex(gIDFTInputTex[uint2(u, y)].xy);
        sum = cadd(sum, CalculateIDFT(fxy, v, y, factor));
    }
    [loop]
    for(uint y = startY + THREAD_SIZE; y < height; ++y) {
        Complex fxy = Complex(gIDFTInputTex[uint2(u, y)].xy);
        sum = cadd(sum, CalculateIDFT(fxy, v, y, factor));
    }
    sum.value /= float(height);

    gIDFTVerticOutputTex[dispatchThreadID.xy] = float4(sum.value.rrr, 1);
}