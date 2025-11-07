#ifndef __FFT_HLSL__
#define __FFT_HLSL__

#include "../Complex.hlsli"

#ifndef THREAD_SIZE
#define THREAD_SIZE 256
#endif

// 蝶形运算
void Butterfly(inout Complex a, inout Complex b, Complex twiddle)
{
    Complex e = a;
    Complex o = b;
    a = cadd(e, cmul(o, twiddle));
    b = csub(e, cmul(o, twiddle));
}

static const float sPI = 3.14159265359f;
static const float sTwoPI = 6.28318530718f;

Texture2D gFFTInputTex : register(t0);
RWTexture2D<float2> gFFTOutputTex : register(u0);
StructuredBuffer<uint> gReverseIndices : register(t1);

#if defined(ENABLE_DEBUG_OUTPUT)
RWTexture2D<float4> gDebugOutputTex : register(u1);
#endif


cbuffer FFTConstants : register(b0)
{
    uint gState;
}

[numthreads(32, 32, 1)]
void ConvertData(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint inputWidth, inputHeight;
    gFFTInputTex.GetDimensions(inputWidth, inputHeight);
    uint outputWidth, outputHeight;
    gFFTOutputTex.GetDimensions(outputWidth, outputHeight);
    if(dispatchThreadID.x >= inputWidth || dispatchThreadID.y >= inputHeight){
        if(dispatchThreadID.x < outputWidth && dispatchThreadID.y < outputHeight){
            gFFTOutputTex[dispatchThreadID.xy] = float2(0, 0);
        }
        return;
    }

    float4 data = gFFTInputTex[dispatchThreadID.xy];
    float luminance = dot(data.rgb, float3(0.299f, 0.587f, 0.114f));
    gFFTOutputTex[dispatchThreadID.xy] = float2(luminance, 0);
}


[numthreads(1, 1, 1)]
void BitReverseCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    gFFTInputTex.GetDimensions(width, height);

    uint2 globalID = dispatchThreadID.xy;
    if (globalID.x >= width)
        return;


#if defined(IS_VERTIC_FFT)
    uint reverseIndex = gReverseIndices[globalID.y];
    // globalID.x = dispatchThreadID.y;
    // globalID.y = dispatchThreadID.x;
    uint2 reverseID = uint2(globalID.x, reverseIndex);
#else
    uint reverseIndex = gReverseIndices[globalID.x];
    uint2 reverseID = uint2(reverseIndex, globalID.y);
#endif
    gFFTOutputTex[globalID] = gFFTInputTex[reverseID].xy;
}

[numthreads(1, 1, 1)]
void HorizFFTCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 globalID = dispatchThreadID.xy;

    uint width, height;
    gFFTOutputTex.GetDimensions(width, height);
    if(globalID.x >= width || globalID.y >= height)
        return;
    
    uint butterStep = 1u << gState;
    uint groupSize = butterStep << 1;   // 每个蝶形组的大小

    uint groupOffset = globalID.x % groupSize;
    if(groupOffset >= butterStep)
        return;
    
    uint index0 = globalID.x;
    uint index1 = index0 + butterStep;
    if(index1 >= width)
        return;

    float angle = -sTwoPI * groupOffset / groupSize;
    Complex twiddle = cexp(angle);

    uint2 texIndex0 = uint2(index0, globalID.y);
    uint2 texIndex1 = uint2(index1, globalID.y);
    Complex fEven = Complex(gFFTInputTex[texIndex0].xy);
    Complex fOdd = Complex(gFFTInputTex[texIndex1].xy);

    Butterfly(fEven, fOdd, twiddle);

    gFFTOutputTex[texIndex0] = fEven.value;
    gFFTOutputTex[texIndex1] = fOdd.value;
}


[numthreads(1, 1, 1)]
void VerticFFTCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 globalID = dispatchThreadID.xy;

    uint width, height;
    gFFTOutputTex.GetDimensions(width, height);
    if(globalID.x >= width || globalID.y >= height)
        return;

    uint butterStep = 1u << gState;
    uint groupSize = butterStep << 1;   // 每个蝶形组的大小

    uint groupOffset = globalID.y % groupSize;
    if(groupOffset >= butterStep)
        return;

    uint index0 = globalID.y;
    uint index1 = index0 + butterStep;
    if(index1 >= height)
        return;

    float angle = -sTwoPI * groupOffset / groupSize;
    Complex twiddle = cexp(angle);

    uint2 texIndex0 = uint2(globalID.x, index0);
    uint2 texIndex1 = uint2(globalID.x, index1);
    Complex fEven = Complex(gFFTInputTex[texIndex0].xy);
    Complex fOdd = Complex(gFFTInputTex[texIndex1].xy);

    Butterfly(fEven, fOdd, twiddle);

    gFFTOutputTex[texIndex0] = fEven.value;
    gFFTOutputTex[texIndex1] = fOdd.value;

#if defined(ENABLE_DEBUG_OUTPUT)
    gDebugOutputTex[globalID] = float4(length(gFFTOutputTex[globalID]).rrr / (width + height), 1);
#endif
}


#endif