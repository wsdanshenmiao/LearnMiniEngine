#include "RootSignature.h"
#include "../Utilities/Hash.h"
#include "RenderContext.h"

using Microsoft::WRL::ComPtr;

namespace DSM{
    static std::map<std::size_t, Microsoft::WRL::ComPtr<ID3D12RootSignature>> s_RootSignatureMap{};
    
    void RootParameter::Clear() noexcept
    {
        if (m_RootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
            delete[] m_RootParameter.DescriptorTable.pDescriptorRanges;
        }
        m_RootParameter.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xffffffff;
    }

    void RootParameter::InitAsConstants(std::uint32_t shaderRegister, std::uint32_t numDwords, D3D12_SHADER_VISIBILITY visibility, std::uint32_t space) noexcept
    {
        m_RootParameter.ShaderVisibility = visibility;
        m_RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        m_RootParameter.Constants.Num32BitValues = numDwords;
    }

    void RootParameter::InitAsConstantBuffer(std::uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility, std::uint32_t space) noexcept
    {
        m_RootParameter.ShaderVisibility = visibility;
        m_RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        m_RootParameter.Descriptor.ShaderRegister = shaderRegister;
        m_RootParameter.Descriptor.RegisterSpace = space;
    }

    void RootParameter::InitAsBufferSRV(std::uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility, std::uint32_t space) noexcept
    {
        m_RootParameter.ShaderVisibility = visibility;
        m_RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        m_RootParameter.Descriptor.ShaderRegister = shaderRegister;
        m_RootParameter.Descriptor.RegisterSpace = space;
    }

    void RootParameter::InitAsBufferUAV(std::uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility, std::uint32_t space) noexcept
    {
        m_RootParameter.ShaderVisibility = visibility;
        m_RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        m_RootParameter.Descriptor.ShaderRegister = shaderRegister;
        m_RootParameter.Descriptor.RegisterSpace = space;
    }

    void RootParameter::InitAsDescriptorTable(std::uint32_t rangeCount, D3D12_SHADER_VISIBILITY visibility) noexcept
    {
        m_RootParameter.ShaderVisibility = visibility;
        m_RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        m_RootParameter.DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE[rangeCount];
        m_RootParameter.DescriptorTable.NumDescriptorRanges = rangeCount;
    }

    void RootParameter::SetTableRange(std::uint32_t rangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE type, std::uint32_t shaderRegister, std::uint32_t count, std::uint32_t space)
    {
        ASSERT(m_RootParameter.DescriptorTable.pDescriptorRanges != nullptr, "Descriptor ranges is null");
        
        auto range = const_cast<D3D12_DESCRIPTOR_RANGE*>(m_RootParameter.DescriptorTable.pDescriptorRanges + rangeIndex);
        range->NumDescriptors = count;
        range->RangeType = type;
        range->RegisterSpace = space;
        range->BaseShaderRegister = shaderRegister;
        range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }


    void RootSignature::Reset(std::uint32_t numRootParams, std::uint32_t numStaticSamplers)
    {
        m_RootParameters.resize(numRootParams);
        m_StaticSamplers.resize(numStaticSamplers);
        m_DescriptorTableSize.resize(numRootParams);
        m_NumInitializedStaticSamplers = 0;   
    }

    void RootSignature::InitStaticSampler(std::uint32_t shaderRegister, const D3D12_SAMPLER_DESC& samplerDesc, D3D12_SHADER_VISIBILITY visibility)
    {
        auto& sampler = m_StaticSamplers[m_NumInitializedStaticSamplers++];
        sampler.Filter = samplerDesc.Filter;
        sampler.AddressU = samplerDesc.AddressU;
        sampler.AddressV = samplerDesc.AddressV;
        sampler.AddressW = samplerDesc.AddressW;
        sampler.MaxAnisotropy = samplerDesc.MaxAnisotropy;
        sampler.ComparisonFunc = samplerDesc.ComparisonFunc;
        sampler.ShaderVisibility = visibility;
        sampler.MipLODBias = samplerDesc.MipLODBias;
        sampler.MaxLOD = samplerDesc.MaxLOD;
        sampler.MinLOD = samplerDesc.MinLOD;
        sampler.RegisterSpace = shaderRegister;
        sampler.ShaderRegister = shaderRegister;

        if (sampler.AddressU == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
            sampler.AddressV == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
            sampler.AddressW == D3D12_TEXTURE_ADDRESS_MODE_BORDER) {
            // 若寻址模式为 D3D12_TEXTURE_ADDRESS_MODE_BORDER 则静态采样器的边框颜色需要为特定值
            WARN_ONCE_IF_NOT(            // Transparent Black
                samplerDesc.BorderColor[0] == 0.0f &&
                samplerDesc.BorderColor[1] == 0.0f &&
                samplerDesc.BorderColor[2] == 0.0f &&
                samplerDesc.BorderColor[3] == 0.0f ||
                // Opaque Black
                samplerDesc.BorderColor[0] == 0.0f &&
                samplerDesc.BorderColor[1] == 0.0f &&
                samplerDesc.BorderColor[2] == 0.0f &&
                samplerDesc.BorderColor[3] == 1.0f ||
                // Opaque White
                samplerDesc.BorderColor[0] == 1.0f &&
                samplerDesc.BorderColor[1] == 1.0f &&
                samplerDesc.BorderColor[2] == 1.0f &&
                samplerDesc.BorderColor[3] == 1.0f,
                "Sampler border color does not match static sampler limitations");

            if (samplerDesc.BorderColor[3] == 1) {
                if (samplerDesc.BorderColor[0] == 0.0f) {
                    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
                }
                else {
                    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
                }
            }
            else {
                sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            }
        }
    }

    void RootSignature::Finalize(const std::wstring& name, D3D12_ROOT_SIGNATURE_FLAGS flags)
    {
        if (m_Finalized) return;

        ASSERT(m_NumInitializedStaticSamplers == m_StaticSamplers.size());

        D3D12_ROOT_SIGNATURE_DESC rootSigDesc;
        rootSigDesc.Flags = flags;
        rootSigDesc.pParameters = reinterpret_cast<const D3D12_ROOT_PARAMETER*>(m_RootParameters.data());
        rootSigDesc.NumParameters = m_RootParameters.size();
        rootSigDesc.pStaticSamplers = m_StaticSamplers.data();
        rootSigDesc.NumStaticSamplers = m_StaticSamplers.size();

        // 计算根签名的 Hash 值
        auto hash = Utility::HashState(&rootSigDesc.Flags);
        hash = Utility::HashState(rootSigDesc.pStaticSamplers, rootSigDesc.NumStaticSamplers, hash);
        for (std::size_t i = 0; i < rootSigDesc.NumParameters; ++i) {
            const auto& param = rootSigDesc.pParameters[i];
            // 若是描述符表，每个描述符都需要Hash
            if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
                auto& descriptorTable = param.DescriptorTable;
                ASSERT(descriptorTable.pDescriptorRanges != nullptr);
                
                hash = Utility::HashState(descriptorTable.pDescriptorRanges, descriptorTable.NumDescriptorRanges, hash);

                // 记录当前是何种描述符表
                auto& ranges = descriptorTable.pDescriptorRanges;
                auto& bitMap = ranges->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER ? m_SamplerTableBitMap : m_DescriptorTableBitMap;
                bitMap |= (1 << i);

                // 获取描述符表的大小
                for (std::size_t j = 0; j < descriptorTable.NumDescriptorRanges; ++j) {
                    m_DescriptorTableSize[i] += descriptorTable.pDescriptorRanges[j].NumDescriptors;
                }
            }
            else {
                hash = Utility::HashState(&param);
            }
        }

        // 需要考虑多线程的情况，当多个线程同时创建根签名时，为了防止重复的序列化根签名和创建根签名，需要阻止后续的线程创建根签名
        bool firstCompile = false;
        ID3D12RootSignature** ppRootSignature = nullptr;
        {
            static std::mutex rootSignatureMutex{};
            // 加锁互斥锁
            std::lock_guard<std::mutex>{rootSignatureMutex};
            
            if (auto it = s_RootSignatureMap.find(hash); it != s_RootSignatureMap.end()) {
                ppRootSignature = it->second.GetAddressOf();
            }
            else {
                ppRootSignature = s_RootSignatureMap[hash].GetAddressOf();
                firstCompile = true;
            }
        }

        // 第一个到达的线程才进行根签名的创建
        if (firstCompile) {
            ComPtr<ID3DBlob> serializedRootSig, errorBlob;
            
            ASSERT_SUCCEEDED(D3D12SerializeRootSignature(
                &rootSigDesc,
                D3D_ROOT_SIGNATURE_VERSION_1,
                serializedRootSig.GetAddressOf(),
                errorBlob.GetAddressOf()));

            if (errorBlob != nullptr) {
                ERROR("Failed to serialize root signature" + std::string((char*)errorBlob->GetBufferPointer()));
            }

            ASSERT_SUCCEEDED(g_RenderContext.GetDevice()->CreateRootSignature(0,
                serializedRootSig->GetBufferPointer(),
                serializedRootSig->GetBufferSize(),
                IID_PPV_ARGS(&m_RootSignature)));

            m_RootSignature->SetName(name.c_str());

            // 将 Hash 表中的根签名与之关联
            s_RootSignatureMap[hash].Attach(m_RootSignature);
            ASSERT(m_RootSignature == *ppRootSignature);
        }
        else {
            while (*ppRootSignature == nullptr) {
                std::this_thread::yield();
            }
            m_RootSignature = *ppRootSignature;
        }

        m_Finalized = true;
    }
}
