#pragma once
#ifndef __LINEARALLOCATOR_H__
#define __LINEARALLOCATOR_H__

#include "../Math/MathCommon.h"
#include "../Utilities/Utility.h"

namespace DSM {
    // 对线性资源进行分配的辅助类
    class LinearAllocator
    {
    public:
        LinearAllocator(std::uint64_t maxSize, std::uint64_t startOffset = 0)
            :m_MaxSize(maxSize), m_StartOffset(startOffset){}
        ~LinearAllocator() = default;

        // 返回分配的资源所处的偏移量
        std::uint64_t Allocate(std::uint64_t size, std::uint32_t alignment = 0) noexcept
        {
            auto alignOffset = Math::AlignUp(m_CurrOffset, alignment);
            m_CurrOffset = alignOffset + size;
            return (alignOffset + size) > m_MaxSize ? Utility::INVALID_ALLOC_OFFSET : alignOffset;
        }
        void Clear() noexcept
        {
            m_CurrOffset = m_StartOffset;
        }

        bool Full() const noexcept { return m_CurrOffset >= m_MaxSize; }
        bool Empty() const noexcept { return m_CurrOffset == m_StartOffset; }
        std::uint64_t MaxSize() const noexcept { return m_MaxSize; }
        std::uint64_t UsedSize() const noexcept { return m_CurrOffset; }
        
    private:
        const std::uint64_t m_MaxSize{};      // 最大容量
        const std::uint64_t m_StartOffset{};  // 起始偏移
        std::uint64_t m_CurrOffset{};   // 当前的偏移
    };   
}


#endif