#pragma once
#ifndef __MATHCOMMON_H__
#define __MATHCOMMON_H__

#define INLINE _forceinline

#include <concepts>
#include <utility>
#include <DirectXMath.h>

namespace DSM::Math {
    
    template <typename T> requires std::is_arithmetic_v<T>
    inline constexpr T AlignUp( T value, std::uint64_t alignment ) noexcept
    {
        if (alignment == 0 || alignment == 1) return value;
        else return (T)(((std::uint64_t)value + (alignment - 1)) & ~(alignment - 1));
    }

    template <typename T> requires std::is_arithmetic_v<T>
    inline constexpr T IsAligned(T value, std::uint64_t alignment ) noexcept
    {
        return 0 == ((std::uint64_t)value & (alignment - 1));
    }    

    template <typename T> requires std::is_arithmetic_v<T>
    inline T DivideByMultiple(T value, std::uint64_t alignment)
    {
        return (T)((value + alignment - 1) / alignment);
    }
}

#endif