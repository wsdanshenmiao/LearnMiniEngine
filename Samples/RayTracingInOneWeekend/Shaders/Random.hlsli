#ifndef __RANDOM_HLSLI__
#define __RANDOM_HLSLI__

struct PCGState { uint state; };

static PCGState sGlobalState;

// 初始化随机种子（每条光线独立 RNG）
void PCG_Init(uint2 pixel, uint frameIndex)
{
    sGlobalState.state = pixel.x * 1973 + pixel.y * 9277 + frameIndex * 26699;
    sGlobalState.state ^= (sGlobalState.state << 13); 
    sGlobalState.state ^= (sGlobalState.state >> 17);
    sGlobalState.state ^= (sGlobalState.state << 5);
}

// 生成 0~1 随机 float
float RandomFloat()
{
    sGlobalState.state = sGlobalState.state * 747796405u + 2891336453u; // PCG 步进
    uint word = ((sGlobalState.state >> ((sGlobalState.state >> 28) + 4)) ^ sGlobalState.state) * 277803737u;
    return (word >> 22) * (1.0f / 4194304.0f);
}

float RandomFloat(float min, float max)
{
    return RandomFloat() * (max - min) + min;
}

int RandomInt()
{
    return int(RandomFloat());
}

int RandomInt(int min, int max)
{
    return RandomInt() * (max - min) + min;
}

uint RandomUint()
{
    return uint(RandomFloat());
}

uint RandomUint(uint min, uint max)
{
    return RandomUint() * (max - min) + min;
}

float2 RandomFloat2()
{
    return float2(RandomFloat(), RandomFloat());
}

float2 RandomFloat2(float2 min, float2 max)
{
    return RandomFloat2() * (max - min) + min;
}

float3 RandomFloat3()
{
    return float3(RandomFloat(), RandomFloat(), RandomFloat());
}

float3 RandomFloat3(float3 min, float3 max)
{
    return RandomFloat3() * (max - min) + min;
}

// 生成均匀分布的随机向量
float3 RandomUnitVector()
{
    // 获取二维圆盘上的均匀分布点
    float x1, x2, s;
    do {
        x1 = RandomFloat() * 2.0 - 1.0; // [-1, 1]
        x2 = RandomFloat() * 2.0 - 1.0; // [-1, 1]
        s = x1 * x1 + x2 * x2;
    } while (s >= 1.0); // 这个循环通常很快收敛
    
    // 映射到三维单位球面上
    float sqrt_val = sqrt(1.0 - s);
    return float3(2.0 * x1 * sqrt_val,
                  2.0 * x2 * sqrt_val,
                  1.0 - 2.0 * s);
}

// 在半球体上生成均匀随机方向
float3 RandomOnHemiSphere(float3 normal)
{
    float3 dir = RandomUnitVector();
    dir = dot(dir, normal) < 0 ? -dir : dir;
    return dir;
}

// 单位圆盘上的均匀随机点
float3 RandomInUnitDisk() {
    // 最多尝试4次以找到盘内点
    for(int i = 0; i < 4; ++i){
        float3 vec = float3(RandomFloat(-1,1), RandomFloat(-1,1), 0);
        if (dot(vec, vec) < 1)
            return vec;
    }
    // 这样生成是不均匀的
    return float3(normalize(float2(RandomFloat(-1,1), RandomFloat(-1,1))), 0);
}

// 生成余弦加权的向量
float3 RandomCosineDirection()
{
    const float PI = 3.14159265;
    float r1 = RandomFloat();
    // 随机极角
    float phi = 2 * PI * r1;
    // 随机半径
    float r2 = sqrt(RandomFloat());

    float x = cos(phi) * r2;
    float y = sin(phi) * r2;
    float z = sqrt(1 - r2 * r2);
    return float3(x, y, z);
}


#endif // __RANDOM_HLSLI__