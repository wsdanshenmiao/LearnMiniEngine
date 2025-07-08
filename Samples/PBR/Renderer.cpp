#include "Renderer.h"
#include "Graphics/RenderContext.h"
#include "Graphics/CommandList/GraphicsCommandList.h"
#include "Mesh.h"


namespace DSM {
    // Renderer implementation
    void Renderer::Create()
    {
        if(m_Initialized) return;

        // 分配描述符
        m_ColorTexRTV = g_RenderContext.AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_ColorTexSRV = g_RenderContext.AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_DepthTexDSV = g_RenderContext.AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        m_DepthTexSRV = g_RenderContext.AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        auto& swapChain = g_RenderContext.GetSwapChain();
        CreateResource(swapChain.GetWidth(), swapChain.GetHeight());

        // 创建根签名
        m_CommonRootSig.InitStaticSampler(0, Graphics::SamplerAnisoWrap);
        m_CommonRootSig[kMeshConstants].InitAsConstantBuffer(kMeshConstants);
        m_CommonRootSig[kMaterialConstants].InitAsConstantBuffer(kMaterialConstants);
        m_CommonRootSig[kPassConstants].InitAsConstantBuffer(kPassConstants);
        m_CommonRootSig[kMaterialSRVs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10);
        m_CommonRootSig.Finalize(L"Renderer::CommonRootSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		m_DefaultPSO.SetRootSignature(m_CommonRootSig);
        m_DefaultPSO.SetBlendState(Graphics::DefaultAlphaBlend);
        m_DefaultPSO.SetDepthStencilState(Graphics::ReadWriteDepthStencil);
        m_DefaultPSO.SetRasterizerState(Graphics::DefaultRasterizer);
        m_DefaultPSO.SetRenderTargetFormat(m_ColorTex.GetFormat(), m_DepthTex.GetFormat());
        m_DefaultPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

        ShaderDesc vsDesc{};
        vsDesc.m_Type = ShaderType::Vertex;
        vsDesc.m_Mode = ShaderMode::SM_6_1;
        vsDesc.m_FileName = "Shaders/Lit.hlsl";
        vsDesc.m_EnterPoint = "LitPassVS";
        m_VS = std::make_unique<ShaderByteCode>(vsDesc);

        ShaderDesc psDesc{};
        psDesc = vsDesc;
        psDesc.m_Type = ShaderType::Pixel;
        psDesc.m_EnterPoint = "LitPassPS";
        m_PS = std::make_unique<ShaderByteCode>(psDesc);

        vsDesc.m_Defines.AddDefine("USE_TANGENT", "1");
        m_VSUseTangent = std::make_unique<ShaderByteCode>(vsDesc);
        psDesc.m_Defines.AddDefine("USE_TANGENT", "1");
        m_PSUseTangent = std::make_unique<ShaderByteCode>(psDesc);

        m_Initialized = true;
    }

    void Renderer::Shutdown()
    {
        // 释放描述符
        g_RenderContext.FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_ColorTexRTV);
        g_RenderContext.FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_ColorTexSRV);
        g_RenderContext.FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, m_DepthTexDSV);
        g_RenderContext.FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_DepthTexSRV);
    }

    void Renderer::OnResize(uint32_t width, uint32_t height)
    {
        CreateResource(width, height);
    }


    uint16_t Renderer::GetPSO(uint16_t psoFlags)
    {
        ASSERT(psoFlags & (kHasPosition | kHasNormal | kHasUV), "Mesh must have position, normal and uv!");
        
        auto colorPSO = m_DefaultPSO;

        std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
        inputElements.emplace_back("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT);
        inputElements.emplace_back("TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT);
        inputElements.emplace_back("NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, D3D12_APPEND_ALIGNED_ELEMENT);
        if(psoFlags & kHasTangent) {
            inputElements.emplace_back("TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 3, D3D12_APPEND_ALIGNED_ELEMENT);
        }
        colorPSO.SetInputLayout(inputElements);

        if(psoFlags & kHasTangent) {
            colorPSO.SetVertexShader(*m_VSUseTangent);
            colorPSO.SetPixelShader(*m_PSUseTangent);
        }
        else {
            colorPSO.SetVertexShader(*m_VS);
            colorPSO.SetPixelShader(*m_PS);
        }

        colorPSO.Finalize();

        m_PSOs.push_back(std::move(colorPSO));

        return m_PSOs.size() - 1;
    }

    void Renderer::CreateResource(uint32_t width, uint32_t height)
    {
        auto& swapChain = g_RenderContext.GetSwapChain();

        // 创建渲染相关的资源
        TextureDesc colTexDesc{};
        colTexDesc.m_Width = width;
        colTexDesc.m_Height = height;
        colTexDesc.m_Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        colTexDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        colTexDesc.m_Format = swapChain.GetBackBuffer()->GetDesc().m_Format;
        m_ColorTex.Create(L"Renderer::ColorTexture", colTexDesc);

        D3D12_CLEAR_VALUE clearValue{};
        clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clearValue.DepthStencil = { .Depth = 1, .Stencil = 0 };
        auto depthTexDesc = colTexDesc;
        depthTexDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        depthTexDesc.m_Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        m_DepthTex.Create(L"Renderer::DepthTexture", depthTexDesc, clearValue);

        // 创建资源视图
        m_ColorTex.CreateRenderTargetView(m_ColorTexRTV);
        m_ColorTex.CreateShaderResourceView(m_ColorTexSRV);
        m_DepthTex.CreateDepthStencilView(m_DepthTexDSV);
        m_DepthTex.CreateShaderResourceView(m_DepthTexSRV);
    }

    Renderer::Renderer()
        :m_TextureHeap(L"Renderer::TextureHeap", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096),
        m_CommonRootSig(kNumRootBindings, 1),
        m_DefaultPSO(L"Renderer::DefaultPSO") {}







    // MeshRenderer implementation
    
    void MeshRenderer::Render(GraphicsCommandList &cmdList, PassConstants &passConstants)
    {
        ASSERT(m_DepthTex != nullptr, "Depth texture is not set!");
		ASSERT(m_RenderCamera != nullptr, "Render camera is not set!");

        auto cameraPos = m_RenderCamera->GetPosition();
        passConstants.m_CameraPos[0] = cameraPos.GetX();
        passConstants.m_CameraPos[1] = cameraPos.GetY();
        passConstants.m_CameraPos[2] = cameraPos.GetZ();
        passConstants.m_View = Math::Matrix4::Transpose(m_RenderCamera->GetViewMatrix());
        passConstants.m_ViewInv = Math::Matrix4::InverseTranspose(m_RenderCamera->GetViewMatrix());
        passConstants.m_Proj = Math::Matrix4::Transpose(m_RenderCamera->GetProjMatrix());
        passConstants.m_ProjInv = Math::Matrix4::InverseTranspose(m_RenderCamera->GetProjMatrix());

        cmdList.TransitionResource(*m_DepthTex, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        cmdList.ClearDepth(m_DepthTexDSV);
        
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs(m_NumRenderTargets);
        for(int i = 0; i < m_NumRenderTargets; ++i)
        {
            ASSERT(m_RenderTarget[i] != nullptr, "Render target is not set!");
            cmdList.TransitionResource(*m_RenderTarget[i], D3D12_RESOURCE_STATE_RENDER_TARGET);
            cmdList.ClearRenderTarget(m_RenderTargetRTV[i]);
            rtvs[i] = m_RenderTargetRTV[i];
        }
        cmdList.SetRenderTargets(rtvs, m_DepthTexDSV);

        cmdList.SetRootSignature(g_Renderer.m_CommonRootSig);
        cmdList.SetDescriptorHeap(g_Renderer.m_TextureHeap.GetHeap());

        cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        cmdList.SetDynamicConstantBuffer(Renderer::kPassConstants, sizeof(passConstants), &passConstants);

        cmdList.SetViewportAndScissor(m_RenderCamera->GetViewPort(), m_Scissor);
        
        for(const auto& [key, obj] : m_SortObjects){
            auto& mesh = *obj.m_Mesh;

            cmdList.SetPipelineState(g_Renderer.m_PSOs[mesh.m_PSOIndex]);

            cmdList.SetConstantBuffer(Renderer::kMeshConstants, obj.m_MeshCBV);
            cmdList.SetConstantBuffer(Renderer::kMaterialConstants, obj.m_MaterialCBV);

            std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexData(3);
            vertexData[0] = mesh.m_PositionStream;
            vertexData[1] = mesh.m_UVStream;
            vertexData[2] = mesh.m_NormalStream;
            if(mesh.m_PSOFlags & kHasTangent){
                vertexData.push_back(mesh.m_TangentStream);
            } 
            cmdList.SetVertexBuffers(0, vertexData);
            cmdList.SetIndexBuffer(mesh.m_IndexBufferViews);

            for(const auto& [name, submesh] : mesh.m_SubMeshes){
                auto handle = g_Renderer.m_TextureHeap[submesh.m_SRVTableOffset];

                cmdList.SetDescriptorTable(Renderer::kMaterialSRVs, handle);
                cmdList.DrawIndexed(submesh.m_IndexCount, 
                    submesh.m_IndexOffset, submesh.m_VertexOffset);
            }
        }
    }

    DirectX::BoundingFrustum MeshRenderer::GetViewFrustum() const
    {
        DirectX::BoundingFrustum frustum{};
        DirectX::BoundingFrustum::CreateFromMatrix(frustum, m_RenderCamera->GetViewProjMatrix());
        return frustum;
    }

    void MeshRenderer::AddRenderTarget(Texture &renderTarget, DescriptorHandle rtv)
    {
        ASSERT(rtv.IsValid());
        m_RenderTarget[m_NumRenderTargets] = &renderTarget;
        m_RenderTargetRTV[m_NumRenderTargets++] = rtv;
    }
    
    void MeshRenderer::SetDepthTexture(Texture &depthTex, DescriptorHandle dsv)
    {
        ASSERT(dsv.IsValid());
        m_DepthTex = &depthTex;
        m_DepthTexDSV = dsv;
    }

    void MeshRenderer::AddMesh(const Mesh &mesh, float distance, 
        D3D12_GPU_VIRTUAL_ADDRESS meshCBV, 
        D3D12_GPU_VIRTUAL_ADDRESS materialCBV)
    {
        SortObject obj{};
        obj.m_Mesh = &mesh;
        obj.m_MeshCBV = meshCBV;
        obj.m_MaterialCBV = materialCBV;

        SortKey key{};
        key.m_ObjIndex = m_SortObjects.size();
        key.m_PSOIndex = mesh.m_PSOIndex;
        key.m_Key = static_cast<uint64_t>(distance * 1000); 

        m_SortObjects.insert({key, obj});
    }
}