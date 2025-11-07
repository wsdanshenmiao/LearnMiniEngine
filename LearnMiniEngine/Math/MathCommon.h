#pragma once
#ifndef __MATHCOMMON_H__
#define __MATHCOMMON_H__

#include <concepts>
#include <utility>
#include "Scalar.h"

namespace DSM::Math {
    class Vector3;
    class Vector4;
    
    class BoolVector
    {
    public:
        __forceinline BoolVector(DirectX::FXMVECTOR v) noexcept : m_Vector(v){}

        __forceinline operator DirectX::XMVECTOR() const noexcept { return m_Vector; }
        __forceinline bool operator==(BoolVector other) const noexcept { return DirectX::XMVector2EqualR(m_Vector, other); }

    private:
        DirectX::XMVECTOR m_Vector{};
    };

    template <typename T>
    concept XMVectorType = std::is_same_v<T, Scalar> || std::is_same_v<T, Vector3> || std::is_same_v<T, Vector4>;
    
    inline uint32_t ReverseBits(uint32_t val, uint32_t numBits)
    {
        val = ((val & 0xaaaaaaaa) >> 1) | ((val & 0x55555555) << 1);
        val = ((val & 0xcccccccc) >> 2) | ((val & 0x33333333) << 2);
        val = ((val & 0xf0f0f0f0) >> 4) | ((val & 0x0f0f0f0f) << 4);
        val = ((val & 0xff00ff00) >> 8) | ((val & 0x00ff00ff) << 8);
        val = (val >> 16) | (val << 16);
        return val >> (32 - numBits);
    }

    template <std::unsigned_integral T>
    inline T NextPowerOf2(T val)
    {
        val--;
        val |= val >> 1;
        val |= val >> 2;
        val |= val >> 4;
        val |= val >> 8;
        val |= val >> 16;
        val++;

        return val;
    }
    
    template <typename T> requires std::is_arithmetic_v<T>
    __forceinline constexpr T AlignUp( T value, std::uint64_t alignment ) noexcept
    {
        if (alignment == 0 || alignment == 1) return value;
        else return (T)(((std::uint64_t)value + (alignment - 1)) & ~(alignment - 1));
    }

    template <typename T> requires std::is_arithmetic_v<T>
    __forceinline constexpr T IsAligned(T value, std::uint64_t alignment ) noexcept
    {
        return 0 == ((std::uint64_t)value & (alignment - 1));
    }    

    template <typename T> requires std::is_arithmetic_v<T>
    __forceinline constexpr T DivideByMultiple(T value, std::uint64_t alignment) noexcept
    {
        return (T)((value + alignment - 1) / alignment);
    }

    template <XMVectorType T> __forceinline constexpr T Sqrt(T s) noexcept { return T(DirectX::XMVectorSqrt(s)); }
    // 计算倒数
    template <XMVectorType T> __forceinline constexpr T Reciprocal(T s) noexcept { return T(DirectX::XMVectorReciprocal(s)); }
    template <XMVectorType T> __forceinline constexpr T ReciprocalSqrt(T s) noexcept { return T(DirectX::XMVectorReciprocalSqrt(s)); }
    template <XMVectorType T> __forceinline constexpr T Floor(T s) noexcept { return T(DirectX::XMVectorFloor(s)); }
    template <XMVectorType T> __forceinline constexpr T Ceiling(T s) noexcept { return T(DirectX::XMVectorCeiling(s)); }
    template <XMVectorType T> __forceinline constexpr T Round(T s) noexcept { return T(DirectX::XMVectorRound(s)); }
    template <XMVectorType T> __forceinline constexpr T Abs(T s) noexcept { return T(DirectX::XMVectorAbs(s)); }
    template <XMVectorType T> __forceinline constexpr T Exp(T s) noexcept { return T(DirectX::XMVectorExp(s)); }
    template <XMVectorType T> __forceinline constexpr T Pow(T b, T e) noexcept { return T(DirectX::XMVectorPow(b, e)); }
    template <XMVectorType T> __forceinline constexpr T Log(T s) noexcept { return T(DirectX::XMVectorLog(s)); }
    template <XMVectorType T> __forceinline constexpr T Sin(T s) noexcept { return T(DirectX::XMVectorSin(s)); }
    template <XMVectorType T> __forceinline constexpr T Cos(T s) noexcept { return T(DirectX::XMVectorCos(s)); }
    template <XMVectorType T> __forceinline constexpr T Tan(T s) noexcept { return T(DirectX::XMVectorTan(s)); }
    template <XMVectorType T> __forceinline constexpr T ASin(T s) noexcept { return T(DirectX::XMVectorASin(s)); }
    template <XMVectorType T> __forceinline constexpr T ACos(T s) noexcept { return T(DirectX::XMVectorACos(s)); }
    template <XMVectorType T> __forceinline constexpr T ATan(T s) noexcept { return T(DirectX::XMVectorATan(s)); }
    // 计算 Y/X 的反正切值
    template <XMVectorType T> __forceinline constexpr T ATan2(T y, T x) noexcept { return T(DirectX::XMVectorATan2(y, x)); }
    template <XMVectorType T> __forceinline constexpr T Lerp(T a, T b, T t) noexcept { return T(DirectX::XMVectorLerpV(a, b, t)); }
    template <XMVectorType T> __forceinline constexpr T Lerp(T a, T b, float t) noexcept { return T(DirectX::XMVectorLerp(a, b, t)); }
    template <XMVectorType T> __forceinline constexpr T Max(T a, T b) noexcept { return T(DirectX::XMVectorMax(a, b)); }
    template <XMVectorType T> __forceinline constexpr T Min(T a, T b) noexcept { return T(DirectX::XMVectorMin(a, b)); }
    template <XMVectorType T> __forceinline constexpr T Clamp(T v, T a, T b) noexcept { return T(DirectX::XMVectorClamp(v, a, b)); }
    template <XMVectorType T> __forceinline constexpr BoolVector operator<(T l, T r) noexcept { return BoolVector(DirectX::XMVectorLess(l, r)); }
    template <XMVectorType T> __forceinline constexpr BoolVector operator<=(T l, T r) noexcept { return BoolVector(DirectX::XMVectorLessOrEqual(l, r)); }
    template <XMVectorType T> __forceinline constexpr BoolVector operator>(T l, T r) noexcept { return BoolVector(DirectX::XMVectorGreater(l, r)); }
    template <XMVectorType T> __forceinline constexpr BoolVector operator>=(T l, T r) noexcept { return BoolVector(DirectX::XMVectorGreaterOrEqual(l, r)); }
    template <XMVectorType T> __forceinline constexpr T Select(T l, T r, BoolVector mask) noexcept { return T(DirectX::XMVectorSelect(l, r, mask)); }

    __forceinline float Sqrt( float s ) { return Sqrt(Scalar(s)); }
    __forceinline float Recip( float s ) { return Reciprocal(Scalar(s)); }
    __forceinline float RecipSqrt( float s ) { return ReciprocalSqrt(Scalar(s)); }
    __forceinline float Floor( float s ) { return Floor(Scalar(s)); }
    __forceinline float Ceiling( float s ) { return Ceiling(Scalar(s)); }
    __forceinline float Round( float s ) { return Round(Scalar(s)); }
    __forceinline float Abs( float s ) { return s < 0.0f ? -s : s; }
    __forceinline float Exp( float s ) { return Exp(Scalar(s)); }
    __forceinline float Pow( float b, float e ) { return Pow(Scalar(b), Scalar(e)); }
    __forceinline float Log( float s ) { return Log(Scalar(s)); }
    __forceinline float Sin( float s ) { return Sin(Scalar(s)); }
    __forceinline float Cos( float s ) { return Cos(Scalar(s)); }
    __forceinline float Tan( float s ) { return Tan(Scalar(s)); }
    __forceinline float ASin( float s ) { return ASin(Scalar(s)); }
    __forceinline float ACos( float s ) { return ACos(Scalar(s)); }
    __forceinline float ATan( float s ) { return ATan(Scalar(s)); }
    __forceinline float ATan2( float y, float x ) { return ATan2(Scalar(y), Scalar(x)); }
    __forceinline float Lerp( float a, float b, float t ) { return a + (b - a) * t; }
    __forceinline float Max( float a, float b ) { return a > b ? a : b; }
    __forceinline float Min( float a, float b ) { return a < b ? a : b; }
    __forceinline float Clamp( float v, float a, float b ) { return Min(Max(v, a), b); }
}

#endif