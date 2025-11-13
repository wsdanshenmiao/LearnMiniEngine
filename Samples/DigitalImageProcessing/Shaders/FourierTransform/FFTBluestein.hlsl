#include "../Complex.hlsli"

#ifndef THREAD_SIZE
#define THREAD_SIZE 16
#endif

static const float sPI = 3.14159265359f;

Texture2D<float2> gInputTex : register(t0);
RWTexture2D<float2> gOutputTex : register(u0);

cbuffer FFTConstants : register(b0)
{
    float gSign; // FFT 时为负，IFFT 时为正
    float2 pad;
}

[numthreads(THREAD_SIZE, THREAD_SIZE, 1)]
void CalcuSequenceCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 globalID = dispatchThreadID.xy;

    uint outputWidth, outputHeight;
    gOutputTex.GetDimensions(outputWidth, outputHeight);
    if(globalID.x >= outputWidth || globalID.y >= outputHeight)
        return;
    
    uint inputWidth, inputHeight;
    gInputTex.GetDimensions(inputWidth, inputHeight);
    if(globalID.x >= inputWidth || globalID.y >= inputHeight){
        gOutputTex[globalID] = 0;
    }
    else{
        Complex inputVal = Complex(gInputTex[globalID]);
        float angle = gSign * sPI * (float(globalID.x * globalID.x) / inputWidth + float(globalID.y * globalID.y) / inputHeight);
        gOutputTex[globalID] = cmul(inputVal, cexp(angle)).value;
    }
}

[numthreads(THREAD_SIZE, THREAD_SIZE, 1)]
void FrequencyMultiplicationCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 globalID = dispatchThreadID.xy;

    uint width, height;
    gOutputTex.GetDimensions(width, height);
    if(globalID.x >= width || globalID.y >= height)
        return;

    // 频域下的卷积核
    Complex kernel = Complex(gInputTex[globalID]);
    if(sign(gSign) > 0)
        kernel = cconj(kernel);
    Complex sequence = Complex(gOutputTex[globalID]);
    gOutputTex[globalID] = cmul(sequence, kernel).value;
}

[numthreads(THREAD_SIZE, THREAD_SIZE, 1)]
void PhaseFactorCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 globalID = dispatchThreadID.xy;

    uint width, height;
    gOutputTex.GetDimensions(width, height);
    if(globalID.x >= width || globalID.y >= height)
        return;

    Complex inputVal = Complex(gInputTex[globalID]);
    float angle = gSign * sPI * (float(globalID.x * globalID.x) / width + float(globalID.y * globalID.y) / height);

    float2 outputVal = cmul(inputVal, cexp(angle)).value;
    if(sign(gSign) > 0)
        outputVal /= width * height;

    gOutputTex[globalID] = outputVal;
}
