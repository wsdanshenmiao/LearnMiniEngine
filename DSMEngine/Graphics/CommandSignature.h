#pragma once
#ifndef __COMMANDSIGNATURE_H__
#define __COMMANDSIGNATURE_H__

#include <d3d12.h>
#include <vector>
#include <wrl/client.h>
#include "RenderContext.h"
#include "Utilities/Macros.h"


namespace DSM {
    class RootSignature;

    // 命令签名参数
    class IndirectParameter
    {
    public:
        void Draw() noexcept{ m_ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW; }
        void DrawIndex() noexcept { m_ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED; }
        void Dispatch() noexcept { m_ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH; }
        void VertexBufferView(UINT slot) noexcept
        {
            m_ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
            m_ArgumentDesc.VertexBuffer.Slot = slot; 
        }
        void IndexBufferView() noexcept { m_ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW; }
        void Constant(UINT rootIndex, UINT offset, UINT numValue) noexcept
        {
            m_ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
            m_ArgumentDesc.Constant.RootParameterIndex = rootIndex;
            m_ArgumentDesc.Constant.DestOffsetIn32BitValues = offset;
            m_ArgumentDesc.Constant.Num32BitValuesToSet = numValue;
        }
        void ConstantBufferView(UINT rootIndex) noexcept
        {
            m_ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
            m_ArgumentDesc.ConstantBufferView.RootParameterIndex = rootIndex;
        }
        void ShaderResourceView(UINT rootIndex) noexcept
        {
            m_ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
            m_ArgumentDesc.ShaderResourceView.RootParameterIndex = rootIndex;
        }
        void UnorderedAccessView(UINT rootIndex) noexcept
        {
            m_ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
            m_ArgumentDesc.UnorderedAccessView.RootParameterIndex = rootIndex;
        }
        void DispatchRays() noexcept { m_ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS; }
        void DispatchMesh() noexcept { m_ArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH; }

        operator D3D12_INDIRECT_ARGUMENT_DESC() const noexcept { return m_ArgumentDesc; }
        
    private:
        D3D12_INDIRECT_ARGUMENT_DESC m_ArgumentDesc;
    };

    // 命令签名
    class CommandSignature
    {
    public:
        CommandSignature(UINT numParams) noexcept { Reset(numParams); }

        void Reset(UINT numParams) noexcept
        {
            Destroy();
            m_IndirectParameters.resize(numParams);
        }
        void Destroy() noexcept
        {
            m_Finalized = false;
            m_IndirectParameters.clear();
            m_CommandSignature = nullptr;
        }

        ID3D12CommandSignature* GetCommandSignature() const { return m_CommandSignature.Get(); }

        IndirectParameter& operator[](UINT index) noexcept
        {
            ASSERT(index < m_IndirectParameters.size());
            return m_IndirectParameters[index];
        }
        const IndirectParameter& operator[](UINT index) const noexcept
        {
            ASSERT(index < m_IndirectParameters.size());
            return m_IndirectParameters[index];
        }

        void Finalize(const RootSignature* rootSig = nullptr)
        {
            if (m_Finalized) return;

            UINT byteStride = 0;
            bool requiresRootSig = false;

            for (const auto& param : m_IndirectParameters) {
                D3D12_INDIRECT_ARGUMENT_DESC paramDsec = param; 
                switch (paramDsec.Type) {
                    case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW:
                        byteStride += sizeof(D3D12_DRAW_ARGUMENTS); break;
                    case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED:
                        byteStride += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS); break;
                    case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH:
                        byteStride += sizeof(D3D12_DISPATCH_ARGUMENTS); break;
                    case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH:
                        byteStride += sizeof(D3D12_DISPATCH_MESH_ARGUMENTS); break;
                    case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW:
                        byteStride += sizeof(D3D12_VERTEX_BUFFER_VIEW); break;
                    case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW:
                        byteStride += sizeof(D3D12_INDEX_BUFFER_VIEW); break;
                    case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT:
                        byteStride += paramDsec.Constant.Num32BitValuesToSet * 4;
                        requiresRootSig = true; break;
                    case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW:
                    case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW:
                    case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW:
                        byteStride += 8;
                        requiresRootSig = true; break;
                }
            }

            D3D12_COMMAND_SIGNATURE_DESC cmdSignatureDesc = {};
            cmdSignatureDesc.ByteStride = byteStride;
            cmdSignatureDesc.pArgumentDescs = reinterpret_cast<const D3D12_INDIRECT_ARGUMENT_DESC*>(m_IndirectParameters.data());
            cmdSignatureDesc.NumArgumentDescs = (UINT)m_IndirectParameters.size();
            cmdSignatureDesc.NodeMask =1;

            ID3D12RootSignature* rootSignature = rootSig ? rootSig->GetRootSignature() : nullptr;
            if (requiresRootSig) {
                ASSERT(rootSignature != nullptr);
            }
            else {
                rootSignature = nullptr;
            }
            ASSERT_SUCCEEDED(g_RenderContext.GetDevice()->CreateCommandSignature(
                &cmdSignatureDesc,
                rootSignature,
                IID_PPV_ARGS(m_CommandSignature.GetAddressOf())));
            m_CommandSignature->SetName(L"CommandSignature");
            m_Finalized = true;
        }

    private:
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_CommandSignature{};
        std::vector<IndirectParameter> m_IndirectParameters;
        bool m_Finalized = false;
    };

}

#endif