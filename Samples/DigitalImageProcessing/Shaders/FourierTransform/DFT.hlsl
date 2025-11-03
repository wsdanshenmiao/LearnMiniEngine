#include "../Complex.hlsli"

static const float sPI = 3.14159265359f;

Texture2D gDFTInputTex : register(t0);
RWTexture2D<float2> gDFTOutputTex : register(u0);

#if defined(ENABLE_DEBUG_OUTPUT)
RWTexture2D<float4> gDebugOutputTex : register(u1);
#endif


// 水平一维离散傅里叶变换
[numthreads(1, 1, 1)]
void HorizLuminanceDFTCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    gDFTInputTex.GetDimensions(width, height);

    uint u = dispatchThreadID.x;
    uint v = dispatchThreadID.y;
    
    if (u >= width || v >= height)
        return;
    
    Complex sum = Complex(float2(0.0f, 0.0f));

    [loop]
    for (uint x = 0; x < width; ++x) {
        float4 pixel = gDFTInputTex[uint2(x, v)];
        float luminance = dot(pixel.rgb, float3(0.299f, 0.587f, 0.114f));
        Complex fxy = Complex(float2(luminance, 0.0f));

        // 计算幅角
        float angle = -2.0f * sPI * float(u * x) / float(width);
        angle = fmod(angle, 2.0f * sPI);
        Complex expTerm = cexp(angle);
        Complex val = cmul(fxy, expTerm);
        sum = cadd(sum, val);
    }

    gDFTOutputTex[dispatchThreadID.xy] = sum.value;
}

// 垂直一维离散傅里叶变换
[numthreads(1, 1, 1)]
void VerticLuminanceDFTCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    gDFTInputTex.GetDimensions(width, height);

    uint u = dispatchThreadID.x;
    uint v = dispatchThreadID.y;
    
    if (u >= width || v >= height)
        return;
    
    Complex sum = Complex(float2(0.0f, 0.0f));

    [loop]
    for (uint y = 0; y < height; ++y) {
        float2 pixel = gDFTInputTex[uint2(u, y)].xy;
        Complex fxy = Complex(pixel);

        // 计算幅角
        float angle = -2.0f * sPI * float(v * y) / float(height);
        angle = fmod(angle, 2.0f * sPI);
        Complex expTerm = cexp(angle);
        Complex val = cmul(fxy, expTerm);
        sum = cadd(sum, val);
    }

    gDFTOutputTex[dispatchThreadID.xy] = sum.value;
#if defined(ENABLE_DEBUG_OUTPUT)
    gDebugOutputTex[dispatchThreadID.xy] = float4(length(sum.value).rrr / (width + height), 1);
#endif
}



Texture2D<float2> gIDFTInputTex : register(t0);
RWTexture2D<float2> gIDFTHorizOutputTex : register(u0);
RWTexture2D<float4> gIDFTVerticOutputTex : register(u0);

[numthreads(1, 1, 1)]
void HorizLuminanceIDFTCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    gIDFTInputTex.GetDimensions(width, height);

    uint u = dispatchThreadID.x;
    uint v = dispatchThreadID.y;
    
    if (u >= width || v >= height)
        return;
    
    Complex sum = (Complex)0;

    [loop]
    for (uint x = 0; x < width; ++x) {
        float2 pixel = gIDFTInputTex[uint2(x, v)];
        Complex fxy = Complex(pixel);

        // 计算幅角
        float angle = 2.0f * sPI * float(u * x) / (float)width;
        angle = fmod(angle, 2.0f * sPI);
        Complex expTerm = cexp(angle);
        Complex val = cmul(fxy, expTerm);
        sum = cadd(sum, val);
    }
    sum.value /= float(width);

    gIDFTHorizOutputTex[dispatchThreadID.xy] = sum.value;
}

[numthreads(1, 1, 1)]
void VerticLuminanceIDFTCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    gIDFTInputTex.GetDimensions(width, height);

    uint u = dispatchThreadID.x;
    uint v = dispatchThreadID.y;
    
    if (u >= width || v >= height)
        return;
    
    Complex sum = (Complex)0;
    
    [loop]
    for (uint y = 0; y < height; ++y) {
        float2 pixel = gIDFTInputTex[uint2(u, y)];
        Complex fxy = Complex(pixel);

        // 计算幅角
        float angle = 2.0f * sPI * float(v * y) / (float)height;
        angle = fmod(angle, 2.0f * sPI);
        Complex expTerm = cexp(angle);
        Complex val = cmul(fxy, expTerm);
        sum = cadd(sum, val);
    }
    sum.value /= float(height);

    gIDFTVerticOutputTex[dispatchThreadID.xy] = float4(sum.value.rrr, 1);
}