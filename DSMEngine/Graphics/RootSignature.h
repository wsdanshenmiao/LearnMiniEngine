#pragma once
#ifndef __ROOTSIGNATURE_H__
#define __ROOTSIGNATURE_H__

#include "../pch.h"

namespace DSM{

    // 根参数
    class RootParameter
    {
    public:
        RootParameter() noexcept
        {
            m_RootParameter.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xffffffff;
        }
        ~RootParameter() noexcept
        {
            Clear();
        }

        // 清理根参数
        void Clear() noexcept;

        // 初始化为各种类型的根参数，包含根常量，根描述符，根描述符堆
        // 根常量
        void InitAsConstants(std::uint32_t shaderRegister, std::uint32_t numDwords, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, std::uint32_t space = 0) noexcept;
        // 根描述符
        void InitAsConstantBuffer(std::uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, std::uint32_t space = 0) noexcept;
        void InitAsBufferSRV(std::uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, std::uint32_t space = 0) noexcept;
        void InitAsBufferUAV(std::uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, std::uint32_t space = 0) noexcept;
        // 描述符堆
        void InitAsDescriptorTable(std::uint32_t rangeCount, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL) noexcept;
        void SetTableRange(std::uint32_t rangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE type, std::uint32_t shaderRegister, std::uint32_t count, std::uint32_t space = 0);
        void InitAsDescriptorRange(
            D3D12_DESCRIPTOR_RANGE_TYPE type,
            std::uint32_t shaderRegister,
            std::uint32_t count,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL,
            std::uint32_t space = 0)
        {
            InitAsDescriptorTable(1, visibility);
            SetTableRange(0, type, shaderRegister, count, space);
        }

        const D3D12_ROOT_PARAMETER& operator()(void) const;

    protected:
        D3D12_ROOT_PARAMETER m_RootParameter;
    };


    // 根签名
    class RootSignature
    {
    public:
        RootSignature(std::uint32_t numRootParams, std::uint32_t numStaticSamplers)
        {
            Reset(numRootParams, numStaticSamplers);
        }

        void Reset(std::uint32_t numRootParams, std::uint32_t numStaticSamplers);
        
        void InitStaticSampler(std::uint32_t shaderRegister, const D3D12_SAMPLER_DESC& samplerDesc, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
        void Finalize(const std::wstring& name, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

        std::uint32_t GetDescriptorTableBitMap() const { return m_DescriptorTableBitMap; }
        std::uint32_t GetSamplerTableBitMap() const { return m_SamplerTableBitMap; }
        std::uint32_t GetDescriptorTableSize(std::size_t index) const
        {
            ASSERT(index < m_DescriptorTableSize.size());
            return m_DescriptorTableSize[index];
        }
        ID3D12RootSignature* GetRootSignature() const noexcept { return m_RootSignature; };
        
        // 获取根参数
        RootParameter& operator[](std::size_t index)
        {
            ASSERT(index < m_RootParameters.size(), "Index out of bounds!");
            return m_RootParameters[index];
        }
        const RootParameter& operator[](std::size_t index) const
        {
            ASSERT(index < m_RootParameters.size(), "Index out of bounds!");
            return m_RootParameters[index];
        }

        // 销毁所有缓存的根签名
        static void DestroyAll() noexcept;

    protected:
        bool m_Finalized = false;

        std::uint32_t m_NumInitializedStaticSamplers = 0;

        // 所有的根参数
        std::vector<RootParameter> m_RootParameters{};
        // 静态采样器
        std::vector<D3D12_STATIC_SAMPLER_DESC> m_StaticSamplers{};

        // 全局根签名的引用指针
        ID3D12RootSignature* m_RootSignature = nullptr;

        // 描述描述符表在根签名中的位置
        std::uint32_t m_DescriptorTableBitMap{};
        // 采样器在根参数中的位置
        std::uint32_t m_SamplerTableBitMap{};
        std::vector<std::uint32_t> m_DescriptorTableSize{};
    };
    
}

#endif