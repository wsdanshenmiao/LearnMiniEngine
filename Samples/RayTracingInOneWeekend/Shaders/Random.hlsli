#ifndef __RANDOM_HLSLI__
#define __RANDOM_HLSLI__


static const float sPI = 3.14159265;

// 初始化PCG状态
uint PCG_Init(uint2 pixel, uint frameIndex)
{
    uint state = (pixel.x * 1973u + pixel.y * 9277u + frameIndex * 26699u) | 1u;
    return state * 747796405u + 2891336453u;
}

// 生成32位随机整数（会更新 state）
uint RandomUint(inout uint state)
{
    // PCG状态更新 (LCG: state = state * multiplier + increment)
    state = state * 747796405u + 2891336453u;

    // PCG输出函数 (XSH-RR变体)
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

uint RandomUint(inout uint state, uint min, uint max)
{
    uint range = max - min;
    return min + (RandomUint(state) % range);
}

// 生成 0~1 随机 float （[0,1)）
float RandomFloat(inout uint state)
{
    return float(RandomUint(state)) * (1.0f / 4294967296.0f);
}

float RandomFloat(inout uint state, float min, float max)
{
    return lerp(min, max, RandomFloat(state));
}

int RandomInt(inout uint state, int minInclusive, int maxExclusive)
{
    return minInclusive + int(RandomUint(state) % asuint(maxExclusive - minInclusive));
}

float2 RandomFloat2(inout uint state)
{
    return float2(RandomFloat(state), RandomFloat(state));
}

float2 RandomFloat2(inout uint state, float2 min, float2 max)
{
    return RandomFloat2(state) * (max - min) + min;
}

float3 RandomFloat3(inout uint state)
{
    return float3(RandomFloat(state), RandomFloat(state), RandomFloat(state));
}

float3 RandomFloat3(inout uint state, float3 min, float3 max)
{
    return RandomFloat3(state) * (max - min) + min;
}

float4 RandomFloat4(inout uint state)
{
    return float4(RandomFloat(state), RandomFloat(state), RandomFloat(state), RandomFloat(state));
}

float4 RandomFloat4(inout uint state, float4 min, float4 max)
{
    return RandomFloat4(state) * (max - min) + min;
}

// 生成均匀分布的随机向量（单位球面）
float3 RandomUnitVector(inout uint state)
{
    float z = RandomFloat(state) * 2.0 - 1.0;           // [-1, 1]
    float phi = 2.0 * sPI * RandomFloat(state);         // [0, 2π)
    float r = sqrt(max(0.0, 1.0 - z * z));        // 投影到 xy 平面的半径
    float x = r * cos(phi);
    float y = r * sin(phi);
    return float3(x, y, z);  
}

// 在半球体上生成均匀随机方向
float3 RandomOnHemiSphere(inout uint state, float3 normal)
{
    float3 dir = RandomUnitVector(state);
    dir = dot(dir, normal) < 0 ? -dir : dir;
    return dir;
}

// 单位圆盘上的均匀随机点
float2 RandomInUnitDisk(inout uint state)
{
    float radius = RandomFloat(state);
    float angle = RandomFloat(state, 0, 2 * sPI);
    return float2(cos(angle), sin(angle)) * radius;
}

// 生成余弦加权的向量
float3 RandomCosineDirection(inout uint state)
{
    float r1 = RandomFloat(state);
    // 随机极角
    float phi = 2 * sPI * r1;
    // 随机半径
    float r2 = sqrt(RandomFloat(state));

    float x = cos(phi) * r2;
    float y = sin(phi) * r2;
    float z = sqrt(1 - r2 * r2);
    return float3(x, y, z);
}


#endif // __RANDOM_HLSLI__