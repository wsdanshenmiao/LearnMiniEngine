#ifndef __COMPLEX_HLSLI__
#define __COMPLEX_HLSLI__

// 复数类型：使用 float2 表示
// .x = 实部, .y = 虚部
struct Complex
{
    float2 value;
};


// 复数加法
Complex cadd(Complex a, Complex b)
{
    return Complex(a.value + b.value);
}

// 复数减法
Complex csub(Complex a, Complex b)
{
    return Complex(a.value - b.value);
}

// 复数乘法
Complex cmul(Complex a, Complex b)
{
    return Complex(float2(
        dot(a.value, float2(b.value.x, -b.value.y)),
        dot(a.value, float2(b.value.y,  b.value.x))));
}

// e^{iθ} = cos(θ) + i * sin(θ)
Complex cexp(float theta)
{
    return Complex(float2(cos(theta), sin(theta)));
}

// 复数共轭
Complex cconj(Complex a)
{
    return Complex(float2(a.value.x, -a.value.y));
}

#endif // __COMPLEX_HLSLI__