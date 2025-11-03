#include "../Complex.hlsli"

static const float sPI = 3.14159265359f;

Texture2D<float4> gDFTInputTex : register(t0);
RWTexture2D<float2> gDFTOutputTex : register(u0);

[numthreads(1, 1, 1)]
void LuminanceDFTCS(uint3 dispatchThreadID : SV_DispatchThreadID)
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
        [loop]
        for (uint y = 0; y < height; ++y) {
            float4 pixel = gDFTInputTex[uint2(x, y)];
            float luminance = dot(pixel.rgb, float3(0.299f, 0.587f, 0.114f));
            Complex fxy = Complex(float2(luminance, 0.0f));

            // 计算幅角
            float angle = -2.0f * sPI * ((u * x) / (float)width + (v * y) / (float)height);
            angle = fmod(angle, 2.0f * sPI);
            Complex expTerm = cexp(angle);
            Complex val = cmul(fxy, expTerm);
            sum = cadd(sum, val);
        }
    }

    gDFTOutputTex[dispatchThreadID.xy] = sum.value;
}


Texture2D<float2> gIDFTInputTex : register(t0);
RWTexture2D<float4> gIDFTOutputTex : register(u0);

[numthreads(1, 1, 1)]
void LuminanceIDFTCS(uint3 dispatchThreadID : SV_DispatchThreadID)
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
        [loop]
        for (uint y = 0; y < height; ++y) {
            float2 pixel = gIDFTInputTex[uint2(x, y)];
            Complex fxy = Complex(pixel);

            // 计算幅角
            float angle = 2.0f * sPI * ((u * x) / (float)width + (v * y) / (float)height);
            angle = fmod(angle, 2.0f * sPI);
            Complex expTerm = cexp(angle);
            Complex val = cmul(fxy, expTerm);
            sum = cadd(sum, val);
        }
    }
    sum.value /= float(width * height);

    gIDFTOutputTex[dispatchThreadID.xy] = float4(sum.value.rrr, 1);
}