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
#include "Graphics/CommandSignature.h"
#include <iostream>
#include "ModelLoader.h"
#include "Renderer.h"
#include "ConstantData.h"
#include "Geometry.h"
#include "Material.h"

using namespace DSM;
using namespace DirectX;


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
        g_Renderer.Create();

        Geometry::GeometryMesh boxGeometry = Geometry::GeometryGenerator::CreateBox(2, 2, 2, 1);
        auto vertexSize = boxGeometry.m_Vertices.size();
        std::vector<XMFLOAT3> boxPos;
        std::vector<XMFLOAT3> boxNormal;
        std::vector<XMFLOAT2> boxUV;
        for (const auto& vertex : boxGeometry.m_Vertices) {
            boxPos.emplace_back(vertex.m_Position);
            boxNormal.emplace_back(vertex.m_Normal);
            boxUV.emplace_back(vertex.m_TexCoord);
        }

        auto posByteSize = vertexSize * sizeof(XMFLOAT3);
        auto normalByteSize = vertexSize * sizeof(XMFLOAT3);
        auto uvByteSize = vertexSize * sizeof(XMFLOAT2);
        auto indexByteSize = boxGeometry.m_Indices32.size() * sizeof(uint32_t);
        GpuBufferDesc meshBufferDesc{};
        meshBufferDesc.m_Flags = D3D12_RESOURCE_FLAG_NONE;
        meshBufferDesc.m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
        meshBufferDesc.m_Stride = 1;
        meshBufferDesc.m_Size = posByteSize + normalByteSize + uvByteSize + indexByteSize;
        m_BoxMesh.m_MeshData.Create(L"BoxMeshData", meshBufferDesc);

        D3D12_GPU_VIRTUAL_ADDRESS bufferLocation = m_BoxMesh.m_MeshData.GetGpuVirtualAddress();
        std::uint32_t offset = 0;
        CommandList::InitBuffer(m_BoxMesh.m_MeshData, boxPos.data(), posByteSize, offset);
        m_BoxMesh.m_PositionStream = { bufferLocation + offset, (UINT)posByteSize, sizeof(XMFLOAT3) };
        offset += posByteSize;

        CommandList::InitBuffer(m_BoxMesh.m_MeshData, boxNormal.data(), normalByteSize, offset);
        m_BoxMesh.m_NormalStream = { bufferLocation + offset, (UINT)normalByteSize, sizeof(XMFLOAT3) };
        offset += normalByteSize;

        CommandList::InitBuffer(m_BoxMesh.m_MeshData, boxUV.data(), uvByteSize, offset);
        m_BoxMesh.m_UVStream = { bufferLocation + offset, (UINT)uvByteSize, sizeof(XMFLOAT2)};
        offset += uvByteSize;

        CommandList::InitBuffer(m_BoxMesh.m_MeshData, boxGeometry.m_Indices32.data(), indexByteSize, offset);
        m_BoxMesh.m_IndexBufferViews = D3D12_INDEX_BUFFER_VIEW{ bufferLocation + offset, (UINT)indexByteSize, DXGI_FORMAT_R32_UINT };
        offset += indexByteSize;

        D3D12_CPU_DESCRIPTOR_HANDLE defaultTexture[kNumTextures] = {
            Graphics::GetDefaultTexture(Graphics::kWhiteOpaque2D),
            Graphics::GetDefaultTexture(Graphics::kWhiteOpaque2D),
            Graphics::GetDefaultTexture(Graphics::kWhiteOpaque2D),
            Graphics::GetDefaultTexture(Graphics::kWhiteOpaque2D),
            Graphics::GetDefaultTexture(Graphics::kBlackTransparent2D),
            Graphics::GetDefaultTexture(Graphics::kDefaultNormalTex)
        };

        DescriptorHandle texHandle = g_Renderer.m_TextureHeap.Allocate(kNumTextures);
        std::uint32_t destCount = kNumTextures;
        std::uint32_t srcCount[kNumTextures] = { 1,1,1,1,1,1 };
        g_RenderContext.GetDevice()->CopyDescriptors(
            1, &texHandle, &destCount, destCount, defaultTexture, srcCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        
        m_BoxMesh.m_Name = "Box";
        m_BoxMesh.m_PSOFlags = kHasPosition | kHasNormal | kHasUV;
        m_BoxMesh.m_PSOIndex = g_Renderer.GetPSO(m_BoxMesh.m_PSOFlags);
        m_BoxMesh.m_SubMeshes.emplace(m_BoxMesh.m_Name, Mesh::SubMesh{
            .m_IndexCount = (UINT)boxGeometry.m_Indices32.size(),
            .m_IndexOffset = 0,
            .m_VertexOffset = 0,
            .m_MaterialIndex = 0,
            .m_SRVTableOffset = (uint16_t)g_Renderer.m_TextureHeap.GetOffsetOfHandle(texHandle) });


        MaterialConstants materialConstant{};
        GpuBufferDesc bufferDesc = {};
        bufferDesc.m_Size = sizeof(MaterialConstants);
        bufferDesc.m_Stride = sizeof(MaterialConstants);
        bufferDesc.m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
        m_BoxMaterial.Create(L"BoxMat", bufferDesc, &materialConstant);


        std::uint64_t width = g_Renderer.m_SceneColorTexture.GetWidth();
        std::uint32_t height = g_Renderer.m_SceneColorTexture.GetHeight();

		m_Scissor = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        m_Camera = std::make_unique<Camera>();
		m_Camera->SetViewPort(0, 0, static_cast<float>(width), static_cast<float>(height));
        float aspect = float(width) / height;
        m_Camera->SetFrustum(DirectX::XM_PIDIV4, aspect == 0 ? 1 : aspect, 0.1f, 1000.0f);
        m_Camera->SetPosition({ 100, 100, -100 });
        //m_Camera->SetPosition({ 0, 0, -10 });
        m_Camera->LookAt({ 0,0,0 }, { 0,1,0 });

        m_SceneTrans.SetScale({ 0.05f, 0.05f, 0.05f });
        MeshConstants meshConstants{};
		meshConstants.m_World = Math::Matrix4::Transpose(m_SceneTrans.GetLocalToWorld());
        meshConstants.m_WorldIT = Math::Matrix4::InverseTranspose(m_SceneTrans.GetLocalToWorld());
		GpuBufferDesc meshConstantsDesc{};
        meshConstantsDesc.m_Size = sizeof(MeshConstants);
		meshConstantsDesc.m_Stride = sizeof(MeshConstants);
		meshConstantsDesc.m_HeapType = D3D12_HEAP_TYPE_UPLOAD;
		m_MeshConstants.Create(L"MeshConstants", meshConstantsDesc, &meshConstants);

        m_Model = LoadModel("Models//Sponza//sponza.gltf");
    }
    virtual void OnResize(std::uint32_t width, std::uint32_t height) override
    {
        // 更行颜色与深度深度缓冲区
        g_Renderer.OnResize(width, height);
		m_Scissor = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        m_Camera->SetViewPort(0, 0, static_cast<float>(width), static_cast<float>(height));
        float aspect = float(width) / height;
        m_Camera->SetFrustum(DirectX::XM_PIDIV4, aspect == 0 ? 1 : aspect, 0.1f, 1000.0f);
    }
    virtual void Update(float deltaTime) override
    {
        m_PassConstants.m_ShadowTrans = Math::Matrix4::Identity;
		m_PassConstants.m_TotalTime = 0;
		m_PassConstants.m_DeltaTime = deltaTime;
	}
    virtual void RenderScene(RenderContext& renderContext) override
    {
        auto& swapChain = renderContext.GetSwapChain();

        g_Renderer.m_SeparateZPass = false;

        GraphicsCommandList cmdList{ L"Render Scene" };

        MeshSorter sorter{ MeshSorter::kDefault };
        sorter.SetCamera(*m_Camera);
        sorter.SetScissor(m_Scissor);
        sorter.SetDepthStencilTarget(g_Renderer.m_SceneDepthTexture, 
            g_Renderer.m_SceneDepthDSV, g_Renderer.m_SceneDepthDSVReadOnly);

        sorter.AddRenderTarget(g_Renderer.m_SceneColorTexture,
            g_Renderer.m_SceneColorRTV, g_Renderer.m_SceneColorSRV);

        m_Model->Render(sorter, m_MeshConstants, m_SceneTrans);
        /*sorter.AddMesh(m_BoxMesh, 2, 
            m_MeshConstants.GetGpuVirtualAddress(), 
            m_BoxMaterial.GetGpuVirtualAddress());*/

        sorter.Render(MeshSorter::kOpaque, cmdList, m_PassConstants);

        cmdList.CopyResource(*swapChain.GetBackBuffer(), g_Renderer.m_SceneColorTexture);

        cmdList.TransitionResource(*swapChain.GetBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);

        cmdList.ExecuteCommandList();

        swapChain.Present();
    }
    virtual void Cleanup() override
    {
        g_Renderer.Shutdown();
    };

private:
    std::unique_ptr<Camera> m_Camera{};

    D3D12_RECT m_Scissor{};

    Transform m_SceneTrans{};
    GpuBuffer m_MeshConstants{};

    PassConstants m_PassConstants{};

    std::shared_ptr<Model> m_Model{};
    Mesh m_BoxMesh{};
    GpuBuffer m_BoxMaterial{};

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