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

RWTexture2D<float2> gFFTOutputTex : register(u0);
StructuredBuffer<uint> gReverseIndices : register(t0);

#if defined(ENABLE_DEBUG_OUTPUT)
RWTexture2D<float4> gDebugOutputTex : register(u1);
#endif

groupshared Complex DataCache[THREAD_SIZE];

cbuffer FFTConstants : register(b0)
{
    uint gNumStages;
    uint gStage;
    float gSign; // FFT 时为负，IFFT 时为正
}

[numthreads(THREAD_SIZE, 1, 1)]
void BitReverseCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    gFFTOutputTex.GetDimensions(width, height);

    uint2 globalID = dispatchThreadID.xy;
    uint dimension = width;
    bool swap = false;

#if defined(IS_VERTICAL)
    globalID = globalID.yx;
    dimension = height;
    swap = true;
#endif

    if(dispatchThreadID.x >= dimension)
        return;
    uint reverseIndex = gReverseIndices[dispatchThreadID.x];
    uint2 reverseID = uint2(reverseIndex, dispatchThreadID.y);
    if(swap)
        reverseID = reverseID.yx;
    if(dispatchThreadID.x < reverseIndex){
        float2 origin = gFFTOutputTex[globalID].xy;
        gFFTOutputTex[globalID] = gFFTOutputTex[reverseID];
        gFFTOutputTex[reverseID] = origin;
    }
}

[numthreads(THREAD_SIZE, 1, 1)]
void FFTRadix2WithGroupMemCS(
    uint3 groupThreadID : SV_GroupThreadID, 
    uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 globalID = dispatchThreadID.xy;
    uint selectedID = globalID.x;
    uint selectedGroupThreadID = groupThreadID.x;

#if defined(IS_VERTICAL)
    globalID = globalID.yx;
#endif

    uint width, height;
    gFFTOutputTex.GetDimensions(width, height);
    if(globalID.x >= width || globalID.y >= height)
        return;

    DataCache[selectedGroupThreadID] = Complex(gFFTOutputTex[globalID]);
    GroupMemoryBarrierWithGroupSync();

    for(uint stage = 0; stage < gNumStages; ++stage){
        uint butterStep = 1u << (stage + gStage);
        uint groupSize = butterStep << 1;   // 每个蝶形组的大小

        uint groupOffset = selectedGroupThreadID % groupSize;
        uint index1 = selectedGroupThreadID + butterStep;
        if(groupOffset < butterStep && index1 < THREAD_SIZE){
            float angle = gSign * sTwoPI * groupOffset / groupSize;
            Complex twiddle = cexp(angle);

            Complex fEven = DataCache[selectedGroupThreadID];
            Complex fOdd = DataCache[index1];

            Butterfly(fEven, fOdd, twiddle);

            DataCache[selectedGroupThreadID] = fEven;
            DataCache[index1] = fOdd;
        }

        GroupMemoryBarrierWithGroupSync();
    }

    gFFTOutputTex[globalID] = DataCache[selectedGroupThreadID].value;
#if defined(ENABLE_DEBUG_OUTPUT)
    if(gSign == -1){
        gDebugOutputTex[globalID] = float4(length(gFFTOutputTex[globalID]).rrr / (width + height), 1);
    }
#endif
}


[numthreads(THREAD_SIZE, 1, 1)]
void FFTRadix2CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 globalID = dispatchThreadID.xy;
    
    uint width, height;
    gFFTOutputTex.GetDimensions(width, height);

    uint selectedID = globalID.x;
    uint selectedDim = width;

#if defined(IS_VERTICAL)
    globalID = globalID.yx;
    selectedID = globalID.y;
    selectedDim = height;
#endif

    if(globalID.x >= width || globalID.y >= height)
        return;
    
    uint butterStep = 1u << gStage;
    uint groupSize = butterStep << 1;   // 每个蝶形组的大小

    uint groupOffset = selectedID % groupSize;
    if(groupOffset >= butterStep)
        return;
    
    uint index1 = selectedID + butterStep;
    if(index1 >= selectedDim)
        return;

    float angle = gSign * sTwoPI * groupOffset / groupSize;
    Complex twiddle = cexp(angle);

    uint2 texIndex1 = uint2(index1, globalID.y);
#if defined(IS_VERTICAL)
    texIndex1 = uint2(globalID.x, index1);
#endif
    Complex fEven = Complex(gFFTOutputTex[globalID].xy);
    Complex fOdd = Complex(gFFTOutputTex[texIndex1].xy);

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

    gFFTOutputTex[dispatchThreadID.xy] = gFFTOutputTex[dispatchThreadID.xy].xy / float(width * height);
}

