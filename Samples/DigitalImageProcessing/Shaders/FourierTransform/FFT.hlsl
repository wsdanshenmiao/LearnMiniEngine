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
    uint gStage;
    float gSign; // FFT 时为负，IFFT 时为正
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


#if defined(IS_VERTICAL)
[numthreads(1, THREAD_SIZE, 1)]
#else
[numthreads(THREAD_SIZE, 1, 1)]
#endif
void BitReverseCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    gFFTInputTex.GetDimensions(width, height);

    uint selectedID = dispatchThreadID.x;
    uint constantID = dispatchThreadID.y;
    uint dimension = width;
    bool swap = false;

#if defined(IS_VERTICAL)
    selectedID = dispatchThreadID.y;
    constantID = dispatchThreadID.x;
    dimension = height;
    swap = true;
#endif

    if(selectedID >= dimension)
        return;
    uint reverseIndex = gReverseIndices[selectedID];
    uint2 reverseID = uint2(reverseIndex, constantID);
    if(swap)
        reverseID = reverseID.yx;
    gFFTOutputTex[dispatchThreadID.xy] = gFFTInputTex[reverseID].xy;
}

[numthreads(THREAD_SIZE, 1, 1)]
void LuminanceFFTCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 globalID = dispatchThreadID.xy;
    uint selectedID = globalID.x;

#if defined(IS_VERTICAL)
    globalID = globalID.yx;
    selectedID = globalID.y;
#endif

    uint width, height;
    gFFTOutputTex.GetDimensions(width, height);
    if(globalID.x >= width || globalID.y >= height)
        return;
    
    uint butterStep = 1u << gStage;
    uint groupSize = butterStep << 1;   // 每个蝶形组的大小

    uint groupOffset = selectedID % groupSize;
    if(groupOffset >= butterStep)
        return;
    
    uint index1 = selectedID + butterStep;
    if(index1 >= width)
        return;

    float angle = gSign * sTwoPI * groupOffset / groupSize;
    Complex twiddle = cexp(angle);

    uint2 texIndex1 = uint2(index1, globalID.y);
#if defined(IS_VERTICAL)
    texIndex1 = uint2(globalID.x, index1);
#endif
    Complex fEven = Complex(gFFTInputTex[globalID].xy);
    Complex fOdd = Complex(gFFTInputTex[texIndex1].xy);

    Butterfly(fEven, fOdd, twiddle);

    gFFTOutputTex[globalID] = fEven.value;
    gFFTOutputTex[texIndex1] = fOdd.value;

#if defined(ENABLE_DEBUG_OUTPUT)
    if(gSign == -1){
        gDebugOutputTex[globalID] = float4(length(gFFTOutputTex[globalID]).rrr / (width + height), 1);
        gDebugOutputTex[texIndex1] = float4(length(gFFTOutputTex[texIndex1]).rrr / (width + height), 1);
    }
#endif
}


[numthreads(32, 32, 1)]
void IFFTScale(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    gFFTOutputTex.GetDimensions(width, height);
    if(dispatchThreadID.x >= width || dispatchThreadID.y >= height)
        return;

    gFFTOutputTex[dispatchThreadID.xy] = gFFTInputTex[dispatchThreadID.xy].xy / float(width * height);
}


#endif