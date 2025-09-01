struct Viewport
{
    float left;
    float top;
    float right;
    float bottom;
};

struct RayGenConstantBuffer
{
    Viewport viewport;
    Viewport stencil;
};

struct RayPayload
{
    float4 color;
};

RaytracingAccelerationStructure gScene : register(t0);
RWTexture2D<float4> gOutput : register(u0);
ConstantBuffer<RayGenConstantBuffer> gConstants : register(b0);


[shader("raygeneration")]
void RaygenShader()
{

}

[shader("miss")]
void MissShader(inout RayPayload payload)
{

}

[shader("closesthit")]
void ClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrs)
{

}