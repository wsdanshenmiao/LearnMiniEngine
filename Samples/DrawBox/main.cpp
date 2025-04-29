#define DEBUG
#include "Core/GameCore.h"
#include "Graphics/GraphicsCommon.h"
#include "Graphics/PipelineState.h"
#include "Graphics/RenderContext.h"
#include "Graphics/RootSignature.h"
#include "Graphics/ShaderCompiler.h"
#include "Graphics/CommandList/GraphicsCommandList.h"
#include "Graphics/Resource/GpuBuffer.h"
#include "Math/Matrix.h"
#include "Math/Random.h"
#include "Math/Transform.h"
#include "Utilities/Utility.h"
#include <iostream>

using namespace DSM;
using namespace DirectX;

struct ObjectConstants
{
    Math::Matrix4 World;
    Math::Matrix4 WorldInvTranspose;
};

struct PassConstants
{
    Math::Matrix4 View;
    Math::Matrix4 InvView;
    Math::Matrix4 Proj;
    Math::Matrix4 InvProj;
    Math::Vector3 EyePosW;
    float pad;
};

class Sandbox : public GameCore::IGameApp
{
public:
    struct VertexPosColor
    {
        static const auto& GetInputLayout()
        {
            static const std::array<D3D12_INPUT_ELEMENT_DESC, 2> inputLayout = {
                D3D12_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                D3D12_INPUT_ELEMENT_DESC{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            return inputLayout;
        };

        DirectX::XMFLOAT3 m_Pos;
        DirectX::XMFLOAT4 m_Color;
    };
    
    struct SubmeshData
    {
        UINT m_IndexCount = 0;
        UINT m_StarIndexLocation = 0;
        INT m_BaseVertexLocation = 0;

        // 单个物体的包围盒
        DirectX::BoundingBox m_Bound;
    };
    
    virtual void Startup()override
    {
        std::array<VertexPosColor, 8> vertexs = {
            VertexPosColor({ XMFLOAT3(-1.0f, -1.0f, -1.0f),XMFLOAT4(Colors::White) }),
            VertexPosColor({ XMFLOAT3(-1.0f, +1.0f, -1.0f),XMFLOAT4(Colors::Black) }),
            VertexPosColor({ XMFLOAT3(+1.0f, +1.0f, -1.0f),XMFLOAT4(Colors::Red) }),
            VertexPosColor({ XMFLOAT3(+1.0f, -1.0f, -1.0f),XMFLOAT4(Colors::Green) }),
            VertexPosColor({ XMFLOAT3(-1.0f, -1.0f, +1.0f),XMFLOAT4(Colors::Blue) }),
            VertexPosColor({ XMFLOAT3(-1.0f, +1.0f, +1.0f),XMFLOAT4(Colors::Yellow) }),
            VertexPosColor({ XMFLOAT3(+1.0f, +1.0f, +1.0f),XMFLOAT4(Colors::Cyan) }),
            VertexPosColor({ XMFLOAT3(+1.0f, -1.0f, +1.0f),XMFLOAT4(Colors::Magenta) })
        };

        std::array<std::uint16_t, 36> indices ={
            0, 1, 2,
            0, 2, 3,
            4, 6, 5,
            4, 7, 6,
            4, 5, 1,
            4, 1, 0,
            3, 2, 6,
            3, 6, 7,
            1, 5, 6,
            1, 6, 2,
            4, 0, 3,
            4, 3, 7
        };
        
        m_ObjectTransform.SetPosition({ 2.0f, 0.0f, 0.0f });
        m_ObjectTransform.SetScale({ 0.5f, 0.5f, 0.5f });

        TextureDesc dsTexDesc{};
        dsTexDesc.m_Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        dsTexDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        dsTexDesc.m_MipLevels = 1;
        dsTexDesc.m_Format = DXGI_FORMAT_R24G8_TYPELESS;
        dsTexDesc.m_Width = g_RenderContext.GetSwapChain().GetWidth();
        dsTexDesc.m_Height = g_RenderContext.GetSwapChain().GetHeight();
        dsTexDesc.m_SampleDesc = { 1, 0 };
        dsTexDesc.m_DepthOrArraySize = 1;
        D3D12_CLEAR_VALUE clearValue{};
        clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clearValue.DepthStencil.Depth = 1.f;
        clearValue.DepthStencil.Stencil = 0;
        m_DepthStencilTexture.Create(L"DepthStencil", dsTexDesc, clearValue);
        D3D12_DEPTH_STENCIL_VIEW_DESC dsViewDesc{};
        dsViewDesc.Flags = D3D12_DSV_FLAG_NONE;
        dsViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsViewDesc.Texture2D.MipSlice = 0;
        m_DepthStencilHandle = g_RenderContext.AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        g_RenderContext.GetDevice()->CreateDepthStencilView(m_DepthStencilTexture.GetResource(), &dsViewDesc, m_DepthStencilHandle);

        GpuBufferDesc vbDesc;
        vbDesc.m_Size = sizeof(VertexPosColor) * vertexs.size();
        vbDesc.m_Stride = sizeof(VertexPosColor);
        vbDesc.m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
        vbDesc.m_Flags = D3D12_RESOURCE_FLAG_NONE;
        m_VertexBuffer.Create(L"VertexBuffer", vbDesc, vertexs.data());
        m_VertexBufferView.BufferLocation = m_VertexBuffer.GetGpuVirtualAddress();
        m_VertexBufferView.SizeInBytes = vbDesc.m_Size;
        m_VertexBufferView.StrideInBytes = vbDesc.m_Stride;
        GpuBufferDesc ibDesc{};
        ibDesc.m_Size = sizeof(std::uint16_t) * indices.size();
        ibDesc.m_Stride = sizeof(std::uint16_t);
        ibDesc.m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
        m_IndexBuffer.Create(L"IndexBuffer", ibDesc, indices.data());
        m_IndexBufferView.BufferLocation = m_IndexBuffer.GetGpuVirtualAddress();
        m_IndexBufferView.SizeInBytes = ibDesc.m_Size;
        m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
        
        ShaderDesc vsDesc{};
        vsDesc.m_FileName = "Shaders\\Color.hlsl";
        vsDesc.m_EnterPoint = "VS";
        vsDesc.m_Type = ShaderType::Vertex;
        vsDesc.m_Mode = ShaderMode::SM_6_1;
        ShaderByteCode vsByteCode{vsDesc};
        auto psDesc = vsDesc;
        psDesc.m_EnterPoint = "PS";
        psDesc.m_Type = ShaderType::Pixel;
        ShaderByteCode psByteCode{psDesc};

        m_RootSig[0].InitAsConstantBuffer(0);
        m_RootSig[1].InitAsConstantBuffer(1);
        m_RootSig.Finalize(L"ColorRootSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        m_PSO.SetRootSignature(m_RootSig);
        m_PSO.SetBlendState(Graphics::GetDefaultBlendState());
        m_PSO.SetRasterizerState(Graphics::GetDefaultRasterizerState());
        m_PSO.SetDepthStencilState(Graphics::GetDefaultDepthStencilState());
        m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        m_PSO.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT);
        m_PSO.SetInputLayout(VertexPosColor::GetInputLayout());
        m_PSO.SetVertexShader(vsByteCode);
        m_PSO.SetPixelShader(psByteCode);
        m_PSO.Finalize();
    }
    virtual void Update(float deltaTime) override
    {
        auto& swapChain = g_RenderContext.GetSwapChain();
        m_ObjectTransform.Rotate({0,0,0}, {0,0,1}, 0.01);
        XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, swapChain.GetWidth() / swapChain.GetHeight(), 1.0f, 1000.0f);
        m_ObjectConstants.World = Math::Matrix4::Transpose(m_ObjectTransform.GetLocalToWorld());
        m_ObjectConstants.WorldInvTranspose = Math::Matrix4::Transpose(Math::Matrix4::Inverse(m_ObjectConstants.World));
        Transform cameraTrans{};
        cameraTrans.SetPosition(0,0,-10);
        cameraTrans.LookAt({ 0,0,0 });
        m_PassConstants.View = Math::Matrix4::Transpose(cameraTrans.GetWorldToLocal());
        m_PassConstants.Proj = Math::Matrix4::Transpose(Math::Matrix4{proj});
        
    }
    virtual void RenderScene(RenderContext& renderContext) override
    {
        auto& swapChain = renderContext.GetSwapChain();
        
        GraphicsCommandList cmdList{L"Draw Box"};

        cmdList.TransitionResource(*swapChain.GetBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        cmdList.TransitionResource(m_DepthStencilTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        
        cmdList.SetViewport(0, 0, swapChain.GetWidth(), swapChain.GetHeight());
        cmdList.SetScissor(0, 0, swapChain.GetWidth(), swapChain.GetHeight());
        cmdList.ClearRenderTarget(swapChain.GetBackBufferRTV(), Colors::Pink);
        cmdList.ClearDepthStencil(m_DepthStencilHandle);
        
        cmdList.SetRenderTarget(swapChain.GetBackBufferRTV(), m_DepthStencilHandle);
        cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        cmdList.SetRootSignature(m_RootSig);
        cmdList.SetPipelineState(m_PSO);

        cmdList.TransitionResource(m_VertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        cmdList.TransitionResource(m_IndexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);
        cmdList.SetVertexBuffer(0, m_VertexBufferView);
        cmdList.SetIndexBuffer(m_IndexBufferView);
        
        cmdList.SetDynamicConstantBuffer(0, sizeof(ObjectConstants), &m_ObjectConstants);
        cmdList.SetDynamicConstantBuffer(1, sizeof(PassConstants), &m_PassConstants);
        cmdList.DrawIndexed(m_IndexBufferView.SizeInBytes / sizeof(std::uint16_t));

        cmdList.TransitionResource(*swapChain.GetBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);

        cmdList.ExecuteCommandList();
        
        swapChain.Present();
    }
    virtual void Cleanup() override{};

private:
    GpuBuffer m_VertexBuffer{};
    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView{};
    GpuBuffer m_IndexBuffer{};
    D3D12_INDEX_BUFFER_VIEW m_IndexBufferView{};
    Texture m_DepthStencilTexture{};
    DescriptorHandle m_DepthStencilHandle{};
    RootSignature m_RootSig{2, 0};
    GraphicsPSO m_PSO{L"Color PSO"};
    ObjectConstants m_ObjectConstants{};
    PassConstants m_PassConstants{};
    Transform m_ObjectTransform{};
};

int WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    Sandbox sandbox{};
    return GameCore::RunApplication(sandbox, 1024, 768, L"DSMEngine", hInstance, nShowCmd);
}