#pragma once
#ifndef __HASH_H__
#define __HASH_H__

#include <cstdint>

namespace DSM::Utility {
    // 用于内存快的Hash函数，使用 SSE4.2 CRC32 硬件加速的看不懂暂时不使用
    inline std::size_t HashRange(const std::uint32_t* const begin, const std::uint32_t* const end, std::size_t hash)
    {
        for (const std::uint32_t* it = begin; it != end; ++it) {
            hash = 16777619U * hash ^ *it;
        }
        return hash;
    }

    template<typename T>
    inline std::size_t HashState(const T* stateDesc, std::size_t count = 1, std::size_t hash = 2166136261U)
    {
        static_assert((sizeof(T) & 3) == 0 && alignof(T) >= 4, "State object is not word-aligned");
        return HashRange((std::uint32_t*)stateDesc, (std::uint32_t*)(stateDesc + count), hash);
    }
    
}

#endif


