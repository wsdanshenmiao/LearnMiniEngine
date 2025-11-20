#ifndef __RANDOM_HLSLI__
#define __RANDOM_HLSLI__


static const float sPI = 3.14159265;

struct PCGState
{
    uint state;
};

uint Hash(uint x)
{
    x = (x ^ 61u) ^ (x >> 16);
    x *= 9u;
    x ^= (x >> 4);
    x *= 0x27d4eb2du;
    x ^= (x >> 15);
    return x | 1u;
}

// 初始化PCG状态
PCGState PCG_Init(uint2 pixel, uint frameIndex)
{
    PCGState rng;
    rng.state = Hash(pixel.x * 1973u ^ pixel.y * 9277u ^ frameIndex * 26699u);

    // warm up
    [unroll]
    for (int i = 0; i < 3; ++i) {
        rng.state = rng.state * 747796405u + 2891336453u;
    }

    return rng;
}

// 生成32位随机整数（会更新 state）
uint RandomUint(inout PCGState rng)
{
    rng.state = rng.state * 747796405u + 2891336453u;
    uint word = ((rng.state >> ((rng.state >> 28u) + 4u)) ^ rng.state) * 277803737u;
    return (word >> 22u) ^ word;
}

uint RandomUint(inout PCGState rng, uint min, uint max)
{
    if (min >= max)
        return min;

    uint range = max - min + 1u;
    return min + (RandomUint(rng) % range);
}

float RandomFloat(inout PCGState rng)
{
    return float(RandomUint(rng)) * (1.0 / 4294967296.0);
}

float RandomFloat(inout PCGState rng, float min, float max)
{
    return lerp(min, max, RandomFloat(rng));
}

int RandomInt(inout PCGState rng, int min, int max)
{
    if (min >= max)
        return min;

    uint range = uint(max - min) + 1u;
    uint scale = RandomUint(rng) % range;
    return min + int(scale);
}

float2 RandomFloat2(inout PCGState rng)
{
    return float2(RandomFloat(rng), RandomFloat(rng));
}

float2 RandomFloat2(inout PCGState rng, float2 min, float2 max)
{
    return lerp(min, max, RandomFloat2(rng));
}

float3 RandomFloat3(inout PCGState rng)
{
    return float3(RandomFloat(rng), RandomFloat(rng), RandomFloat(rng));
}

float3 RandomFloat3(inout PCGState rng, float3 min, float3 max)
{
    return lerp(min, max, RandomFloat3(rng));
}

float4 RandomFloat4(inout PCGState rng)
{
    return float4(RandomFloat(rng), RandomFloat(rng), RandomFloat(rng), RandomFloat(rng));
}

float4 RandomFloat4(inout PCGState rng, float4 min, float4 max)
{
    return lerp(min, max, RandomFloat4(rng));
}

// 生成均匀分布的随机向量（单位球面）
float3 RandomUnitVector(inout PCGState rng)
{
    float r1 = RandomFloat(rng);
    float r2 = RandomFloat(rng);
    float sinPhi = sqrt((1.0 - r2) * r2);
    float theta = 2.0 * sPI * r1;
    float x = cos(theta) * sinPhi;
    float y = sin(theta) * sinPhi;
    float z = 1 - 2 * r2;
    return float3(x, y, z);
}

// 在半球体上生成均匀随机方向
float3 RandomOnHemiSphere(inout PCGState rng, float3 normal)
{
    float3 dir = RandomUnitVector(rng);
    dir = dot(dir, normal) < 0 ? -dir : dir;
    return dir;
}

// 单位圆盘上的均匀随机点
float2 RandomInUnitDisk(inout PCGState rng)
{
    float radius = sqrt(max(0.0, RandomFloat(rng)));
    float angle = RandomFloat(rng, 0, 2 * sPI);
    return float2(cos(angle), sin(angle)) * radius;
}

// 生成余弦加权的向量
float3 RandomCosineDirection(inout PCGState rng)
{
    float r1 = RandomFloat(rng);
    // 随机极角
    float phi = 2 * sPI * r1;
    // 随机半径
    float r2 = sqrt(RandomFloat(rng));

    float x = cos(phi) * r2;
    float y = sin(phi) * r2;
    float z = sqrt(1 - r2 * r2);
    return float3(x, y, z);
}


#endif // __RANDOM_HLSLI__