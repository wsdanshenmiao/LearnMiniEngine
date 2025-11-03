#include "../Complex.hlsli"

#ifndef THREAD_SIZE
#define THREAD_SIZE 256
#endif

static const float sPI = 3.14159265359f;

Texture2D gDFTInputTex : register(t0);
RWTexture2D<float2> gDFTOutputTex : register(u0);

#if defined(ENABLE_DEBUG_OUTPUT)
RWTexture2D<float4> gDebugOutputTex : register(u1);
#endif


groupshared Complex DataCache[THREAD_SIZE];

Complex CalculateDFT(Complex fxy, float u, float x, float len)
{
    // 计算幅角
    float angle = -2.0f * sPI * (u * x) / len;
    angle = fmod(angle, 2.0f * sPI);
    Complex expTerm = cexp(angle);
    return cmul(fxy, expTerm);
}

Complex CalculateIDFT(Complex fxy, float u, float x, float len)
{
    // 计算幅角
    float angle = 2.0f * sPI * (u * x) / len;
    angle = fmod(angle, 2.0f * sPI);
    Complex expTerm = cexp(angle);
    return cmul(fxy, expTerm);
}

// 水平一维离散傅里叶变换
[numthreads(THREAD_SIZE, 1, 1)]
void HorizLuminanceDFTCS(
    uint3 groupID : SV_GroupID, 
    uint3 groupThreadID : SV_GroupThreadID, 
    uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    gDFTInputTex.GetDimensions(width, height);

    uint u = dispatchThreadID.x;
    uint v = dispatchThreadID.y;
    
    if (u >= width || v >= height)
        return;

    // 储存数据到组内共享内存
    float4 data = gDFTInputTex[uint2(u, v)];
    float luminance = dot(data.rgb, float3(0.299f, 0.587f, 0.114f));
    DataCache[groupThreadID.x] = Complex(float2(luminance, 0.0f));
    GroupMemoryBarrierWithGroupSync();
    
    Complex sum = Complex(float2(0.0f, 0.0f));

    uint startX = groupID.x * THREAD_SIZE;
    // 先计算共享内存中的 DFT
    for (uint i = 0; i < THREAD_SIZE && (startX + i) < width; ++i) {
        sum = cadd(sum, CalculateDFT(DataCache[i], u, i + startX, width));
    }

    [loop]
    for (uint x = 0; x < startX; ++x) {
        float4 pixel = gDFTInputTex[uint2(x, v)];
        float luminance = dot(pixel.rgb, float3(0.299f, 0.587f, 0.114f));
        Complex fxy = Complex(float2(luminance, 0.0f));
        sum = cadd(sum, CalculateDFT(fxy, u, x, width));
    }
    [loop]
    for(uint x = startX + THREAD_SIZE; x < width; ++x) {
        float4 pixel = gDFTInputTex[uint2(x, v)];
        float luminance = dot(pixel.rgb, float3(0.299f, 0.587f, 0.114f));
        Complex fxy = Complex(float2(luminance, 0.0f));
        sum = cadd(sum, CalculateDFT(fxy, u, x, width));
    }

    gDFTOutputTex[dispatchThreadID.xy] = sum.value;
}

// 垂直一维离散傅里叶变换
[numthreads(1, THREAD_SIZE, 1)]
void VerticLuminanceDFTCS(
    uint3 groupID : SV_GroupID, 
    uint3 groupThreadID : SV_GroupThreadID, 
    uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    gDFTInputTex.GetDimensions(width, height);

    uint u = dispatchThreadID.x;
    uint v = dispatchThreadID.y;
    
    if (u >= width || v >= height)
        return;

    // 储存数据到组内共享内存
    DataCache[groupThreadID.y] = Complex(gDFTInputTex[uint2(u, v)].xy);
    GroupMemoryBarrierWithGroupSync();
    
    Complex sum = Complex(float2(0.0f, 0.0f));

    uint startY = groupID.y * THREAD_SIZE;
    // 先计算共享内存中的 DFT
    for (uint i = 0; i < THREAD_SIZE && (startY + i) < height; ++i) {
        sum = cadd(sum, CalculateDFT(DataCache[i], v, i + startY, height));
    }

    [loop]
    for (uint y = 0; y < startY; ++y) {
        Complex fxy = Complex(gDFTInputTex[uint2(u, y)].xy);
        sum = cadd(sum, CalculateDFT(fxy, v, y, height));
    }
    [loop]
    for(uint y = startY + THREAD_SIZE; y < height; ++y) {
        Complex fxy = Complex(gDFTInputTex[uint2(u, y)].xy);
        sum = cadd(sum, CalculateDFT(fxy, v, y, height));
    }

    gDFTOutputTex[dispatchThreadID.xy] = sum.value;
#if defined(ENABLE_DEBUG_OUTPUT)
    gDebugOutputTex[dispatchThreadID.xy] = float4(length(sum.value).rrr / (width + height), 1);
#endif
}



Texture2D<float2> gIDFTInputTex : register(t0);
RWTexture2D<float2> gIDFTHorizOutputTex : register(u0);
RWTexture2D<float4> gIDFTVerticOutputTex : register(u0);

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

    // 储存数据到组内共享内存
    DataCache[groupThreadID.x] = Complex(gIDFTInputTex[uint2(u, v)].xy);
    GroupMemoryBarrierWithGroupSync();

    uint startX = groupID.x * THREAD_SIZE;
    // 先计算共享内存中的 DFT
    for (uint i = 0; i < THREAD_SIZE && (startX + i) < width; ++i) {
        sum = cadd(sum, CalculateIDFT(DataCache[i], u, i + startX, width));
    }

    [loop]
    for (uint x = 0; x < startX; ++x) {
        Complex fxy = Complex(gIDFTInputTex[uint2(x, v)].xy);
        sum = cadd(sum, CalculateIDFT(fxy, u, x, width));
    }
    [loop]
    for(uint x = startX + THREAD_SIZE; x < width; ++x) {
        Complex fxy = Complex(gIDFTInputTex[uint2(x, v)].xy);
        sum = cadd(sum, CalculateIDFT(fxy, u, x, width));
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

    uint startY = groupID.y * THREAD_SIZE;
    // 先计算共享内存中的 DFT
    for (uint i = 0; i < THREAD_SIZE && (startY + i) < height; ++i) {
        sum = cadd(sum, CalculateIDFT(DataCache[i], v, i + startY, height));
    }

    [loop]
    for (uint y = 0; y < startY; ++y) {
        Complex fxy = Complex(gIDFTInputTex[uint2(u, y)].xy);
        sum = cadd(sum, CalculateIDFT(fxy, v, y, height));
    }
    [loop]
    for(uint y = startY + THREAD_SIZE; y < height; ++y) {
        Complex fxy = Complex(gIDFTInputTex[uint2(u, y)].xy);
        sum = cadd(sum, CalculateIDFT(fxy, v, y, height));
    }
    sum.value /= float(height);

    gIDFTVerticOutputTex[dispatchThreadID.xy] = float4(sum.value.rrr, 1);
}