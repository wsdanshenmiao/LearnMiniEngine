Texture2D gInputTex : register(t0);
RWTexture2D<float2> gOutputTex : register(u0);

[numthreads(32, 32, 1)]
void CalculateLuminance(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    float4 data = gInputTex[dispatchThreadID.xy];
    float luminance = dot(data.rgb, float3(0.299f, 0.587f, 0.114f));
    gOutputTex[dispatchThreadID.xy] = float2(luminance, 0);
}