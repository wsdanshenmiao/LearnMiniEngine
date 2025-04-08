#pragma once
#ifndef __ROOTSIGNATURE_H__
#define __ROOTSIGNATURE_H__

#include "../pch.h"

namespace DSM::Graphics{

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
        void InitAsConstants(UINT shaderRegister, UINT numDwords, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0) noexcept;
        // 根描述符
        void InitAsConstantBuffer(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0) noexcept;
        void InitAsBufferSRV(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0) noexcept;
        void InitAsBufferUAV(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0) noexcept;
        // 描述符堆
        void InitAsDescriptorTable(UINT rangeCount, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0) noexcept;
        void SetTableRange(UINT rangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE type, UINT shaderRegister, UINT count, UINT space = 0);
        void InitAsDescriptorRange(
            D3D12_DESCRIPTOR_RANGE_TYPE type,
            UINT shaderRegister,
            UINT count,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL,
            UINT space = 0)
        {
            InitAsDescriptorTable(count, visibility, space);
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
        RootSignature(UINT numRootParams, UINT numStaticSamplers)
        {
            Reset(numRootParams, numStaticSamplers);
        }

        void Reset(UINT numRootParams, UINT numStaticSamplers)
        {
            m_RootParameters.clear();
            m_StaticSamplers.clear();
            m_NumInitializedStaticSamplers = 0;    
        }
        
        void InitStaticSampler(UINT shaderRegister, const D3D12_SAMPLER_DESC& samplerDesc, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
        void Finalize(const std::wstring& name, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
        
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
        static void DestroyAll() noexcept
        {
            sm_RootSignatureMap.clear();
        }

    protected:
        bool m_Finalized = false;

        UINT m_NumInitializedStaticSamplers = 0;

        // 所有的根参数
        std::vector<RootParameter> m_RootParameters{};
        // 静态采样器
        std::vector<D3D12_STATIC_SAMPLER_DESC> m_StaticSamplers{};

        // 全局根签名的引用指针
        ID3D12RootSignature* m_RootSignature = nullptr;

        inline static std::map<std::uint32_t, Microsoft::WRL::ComPtr<ID3D12RootSignature>> sm_RootSignatureMap;
    };
    
}

#endif