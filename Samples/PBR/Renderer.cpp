#include "Renderer.h"

#include "ConstantData.h"
#include "Mesh.h"
#include "Renderer/TextureManager.h"
#include "Graphics/GraphicsCommon.h"
#include "Graphics/RenderContext.h"
#include "Graphics/CommandList/GraphicsCommandList.h"

namespace DSM {
    void Renderer::Create()
    {
        if (m_Initialized) return;

        // 创建通用根签名
        m_RootSignature.Reset(kNumRootBindings, 2);
        // 通用的采样器
        m_RootSignature.InitStaticSampler(0, Graphics::SamplerAnisoWrap, D3D12_SHADER_VISIBILITY_PIXEL);
        // 深度图的采样器
        m_RootSignature.InitStaticSampler(1, Graphics::SamplerShadow, D3D12_SHADER_VISIBILITY_PIXEL);
        m_RootSignature[kMeshConstants].InitAsConstantBuffer(kMeshConstants);
        m_RootSignature[kMaterialConstants].InitAsConstantBuffer(kMaterialConstants);
        m_RootSignature[kPassConstants].InitAsConstantBuffer(kPassConstants);
        m_RootSignature[kMaterialSRVs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10, D3D12_SHADER_VISIBILITY_PIXEL);
		m_RootSignature[kCommonSRVs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 10, D3D12_SHADER_VISIBILITY_PIXEL);
        m_RootSignature.Finalize(L"RendererRootSignature", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        const auto& swapChain = g_RenderContext.GetSwapChain();
        m_CommonTexture = m_TextureHeap.Allocate(1);
        m_SceneColorSRV = g_RenderContext.AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_SceneColorRTV = g_RenderContext.AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_SceneDepthSRV = g_RenderContext.AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_SceneDepthDSV = g_RenderContext.AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        m_ShadowMapSRV = g_RenderContext.AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_ShadowMapDSV = g_RenderContext.AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        m_ShadowMapDSVReadOnly = g_RenderContext.AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		OnResize(swapChain.GetWidth(), swapChain.GetHeight());

        //D3D12_INPUT_ELEMENT_DESC posOnly[] = {
        //    {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        //};
        //D3D12_INPUT_ELEMENT_DESC posAndUV[] = {
        //    {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        //    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        //};


        // 编译Shader
		ShaderDesc litVSDesc{
			.m_Type = ShaderType::Vertex,
			.m_Mode = ShaderMode::SM_6_1,
			.m_FileName = "Shaders\\Lit.hlsl",
			.m_EnterPoint = "LitPassVS"
		};
		ShaderDesc litPSDesc{
			.m_Type = ShaderType::Pixel,
			.m_Mode = ShaderMode::SM_6_1,
			.m_FileName = "Shaders\\Lit.hlsl",
			.m_EnterPoint = "LitPassPS"
		};
        ShaderDesc litUseTangentVSDesc = litVSDesc;
        litUseTangentVSDesc.m_Defines = ShaderDefines{ {"USE_TANGENT", "1"} };
		ShaderDesc litUseTangentPSDesc = litPSDesc;
		litUseTangentPSDesc.m_Defines = ShaderDefines{ {"USE_TANGENT", "1"} };
        m_LitVS = std::make_unique<ShaderByteCode>(litUseTangentVSDesc);
        m_LitPS = std::make_unique<ShaderByteCode>(litUseTangentPSDesc);
		m_LitNoTangentVS = std::make_unique<ShaderByteCode>(litVSDesc);
		m_LitNoTangentPS = std::make_unique<ShaderByteCode>(litPSDesc);

  //      // 仅深度写入
		//ShaderByteCode depthOnlyVS{ ShaderDesc{
		//	.m_Type = ShaderType::Vertex,
		//	.m_Mode = ShaderMode::SM_6_1,
		//	.m_FileName = "Shaders\\DepthOnly.hlsl",
		//	.m_EnterPoint = "DepthOnlyPassVS"
		//} };
  //      ShaderDesc depthOnlyPSDesc{
  //          .m_Type = ShaderType::Pixel,
  //          .m_Mode = ShaderMode::SM_6_1,
  //          .m_FileName = "Shaders\\DepthOnly.hlsl",
  //          .m_EnterPoint = "DepthOnlyPassPS"
  //      };
		//ShaderByteCode depthOnlyPS{depthOnlyPSDesc};
  //      GraphicsPSO depthOnlyPSO{L"DepthOnlyPSO"};
  //      depthOnlyPSO.SetRootSignature(m_RootSignature);
  //      depthOnlyPSO.SetRasterizerState(Graphics::DefaultRasterizer);
  //      depthOnlyPSO.SetBlendState(Graphics::DisableBlend);
  //      depthOnlyPSO.SetDepthStencilState(Graphics::ReadWriteDepthStencil);
  //      depthOnlyPSO.SetInputLayout(posAndUV);
  //      depthOnlyPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  //      depthOnlyPSO.SetRenderTargetFormats(0, nullptr, m_SceneDepthTexture.GetFormat());
  //      // TODO:编写深度写入的Shader之后插入绑定着色器
		//depthOnlyPSO.SetVertexShader(depthOnlyVS);
		//depthOnlyPSO.SetPixelShader(depthOnlyPS);
  //      depthOnlyPSO.Finalize();
  //      m_PSOs.push_back(depthOnlyPSO);

  //      // AlphaTest的深度写入
  //      auto depthOnlyAlphaTestPSDesc = depthOnlyPSDesc;
		//depthOnlyAlphaTestPSDesc.m_Defines = ShaderDefines{ { "ALPHA_TEST", "1" } };
  //      ShaderByteCode depthOnlyAlphaTestPS{ depthOnlyAlphaTestPSDesc };
  //      GraphicsPSO cutoutDepthOnlyPSO{L"CutoutDepthOnlyPSO"};
  //      cutoutDepthOnlyPSO = depthOnlyPSO;
  //      cutoutDepthOnlyPSO.SetInputLayout(posAndUV);
  //      cutoutDepthOnlyPSO.SetRasterizerState(Graphics::BothSidedRasterizer);
  //      // TODO:后续插入绑定着色器
  //      cutoutDepthOnlyPSO.SetVertexShader(depthOnlyVS);
  //      cutoutDepthOnlyPSO.SetPixelShader(depthOnlyAlphaTestPS);
  //      cutoutDepthOnlyPSO.Finalize();
  //      m_PSOs.push_back(cutoutDepthOnlyPSO);
        
        
        //// 绘制ShadowMap时使用
        //depthOnlyPSO.SetRasterizerState(Graphics::ShadowRasterizer);
        //depthOnlyPSO.SetRenderTargetFormats(0, nullptr, m_ShadowMap.GetFormat());
        //depthOnlyPSO.Finalize();
        //m_PSOs.push_back(depthOnlyPSO);

        //GraphicsPSO cutoutShadowMapPSO{L"CutoutShadowMapPSO"};
        //cutoutShadowMapPSO = cutoutDepthOnlyPSO;
        //depthOnlyPSO.SetRasterizerState(Graphics::ShadowBothSidedRasterizer);
        //depthOnlyPSO.SetRenderTargetFormats(0, nullptr, m_ShadowMap.GetFormat());
        //cutoutDepthOnlyPSO.Finalize();
        //m_PSOs.push_back(cutoutShadowMapPSO);

        m_DefaultPSO.SetRootSignature(m_RootSignature);
        m_DefaultPSO.SetBlendState(Graphics::DefaultAlphaBlend);
        m_DefaultPSO.SetRasterizerState(Graphics::DefaultRasterizer);
        m_DefaultPSO.SetDepthStencilState(Graphics::ReadWriteDepthStencil);
        m_DefaultPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        m_DefaultPSO.SetRenderTargetFormats(1, &m_SceneColorTexture.GetFormat(), m_SceneDepthTexture.GetFormat());
        m_DefaultPSO.SetInputLayout({});
        m_DefaultPSO.SetVertexShader(*m_LitNoTangentVS);
        m_DefaultPSO.SetPixelShader(*m_LitNoTangentPS);
        
        
        //m_SkyboxPSO = m_DefaultPSO;
        //m_SkyboxPSO.SetDepthStencilState(Graphics::ReadOnlyDepthStencil);
        //m_SkyboxPSO.SetInputLayout({});
        //// TODO:添加着色器绑定
        //m_SkyboxPSO.Finalize();


        m_Initialized = true;
    }

    void Renderer::Shutdown()
    {
        m_Initialized = false;
        m_TextureHeap.Clear();
        m_ShadowMap.GetDesc();
        m_SceneColorTexture.Destroy();
        m_SceneDepthTexture.Destroy();
        g_RenderContext.FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_SceneColorSRV);
        g_RenderContext.FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_SceneColorRTV);
        g_RenderContext.FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_SceneDepthSRV);
        g_RenderContext.FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, m_SceneDepthDSV);
        g_RenderContext.FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_ShadowMapSRV);
        g_RenderContext.FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, m_ShadowMapDSV);
    }

    std::uint16_t Renderer::GetPSO(std::uint16_t psoFlags)
    {
        GraphicsPSO colorPSO{L"ColorPSO" + std::to_wstring(m_PSOs.size())};
        colorPSO = m_DefaultPSO;
        
        std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayouts{};
        if (psoFlags & kHasPosition) {
            inputLayouts.emplace_back(D3D12_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        }
        if (psoFlags & kHasUV) {
            inputLayouts.emplace_back(D3D12_INPUT_ELEMENT_DESC{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        }
        if (psoFlags & kHasNormal) {
            inputLayouts.emplace_back(D3D12_INPUT_ELEMENT_DESC{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        }
        if (psoFlags & kHasTangent) {
            inputLayouts.emplace_back(D3D12_INPUT_ELEMENT_DESC{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        }
        colorPSO.SetInputLayout({inputLayouts.data(), inputLayouts.size()});

        // TODO：编写完PBR后插入着色器的代码
		if (psoFlags & kHasTangent) {
			colorPSO.SetVertexShader(*m_LitVS);
			colorPSO.SetPixelShader(*m_LitPS);
		}
		else {
			colorPSO.SetVertexShader(*m_LitNoTangentVS);
			colorPSO.SetPixelShader(*m_LitNoTangentPS);
		}
        
        //if (psoFlags & kBothSide) {
        //    colorPSO.SetRasterizerState(Graphics::BothSidedRasterizer);
        //}
        //if (psoFlags & kAlphaBlend) {
        //    colorPSO.SetBlendState(Graphics::PreMultipliedBlend);
        //}
        colorPSO.Finalize();

        for (std::size_t i = 0; i < m_PSOs.size(); ++i) {
            if (m_PSOs[i].GetPipelineStateObject() == colorPSO.GetPipelineStateObject()) {
                return i;
            }
        }

        m_PSOs.push_back(colorPSO);

        //// PreZ Pass
        //colorPSO.SetDepthStencilState(Graphics::TestEqualDepthStencil);
        //colorPSO.Finalize();
        //m_PSOs.push_back(colorPSO);
        
        return m_PSOs.size() - 1;
    }

    void Renderer::OnResize(std::uint32_t width, std::uint32_t height)
    {
		width = max(width, 1u);
		height = max(height, 1u);
        TextureDesc texDesc{};
        texDesc.m_Width = width;
        texDesc.m_Height = height;
        texDesc.m_Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.m_Format = g_RenderContext.GetSwapChain().GetBackBuffer()->GetFormat();
        texDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        D3D12_CLEAR_VALUE clearValue{};
        clearValue.Format = texDesc.m_Format;
        m_SceneColorTexture.Create(L"Renderer::SceneColorTexture", texDesc, clearValue);
        m_SceneColorTexture.CreateShaderResourceView(m_SceneColorSRV);
        m_SceneColorTexture.CreateRenderTargetView(m_SceneColorRTV);

        texDesc.m_Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        texDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clearValue.DepthStencil.Depth = 1.f;
        clearValue.DepthStencil.Stencil = 0;
        m_SceneDepthTexture.Create(L"Renderer::SceneDepthTexture", texDesc, clearValue);
        m_SceneDepthDSVReadOnly = g_RenderContext.AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        m_SceneDepthTexture.CreateShaderResourceView(m_SceneDepthSRV);
        m_SceneDepthTexture.CreateDepthStencilView(m_SceneDepthDSV);
        m_SceneDepthTexture.CreateDepthStencilView(m_SceneDepthDSVReadOnly, D3D12_DSV_FLAG_READ_ONLY_DEPTH);

        ResizeShadowMap(width, height);
    }
    
    void Renderer::ResizeShadowMap(std::uint32_t width, std::uint32_t height)
    {
        ASSERT(width > 0 && height > 0);
        TextureDesc shadowMapDesc = {};
        shadowMapDesc.m_Width = width;
        shadowMapDesc.m_Height = height;
        shadowMapDesc.m_Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        shadowMapDesc.m_Format = DXGI_FORMAT_R24G8_TYPELESS;
        shadowMapDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        m_ShadowMap.Create(L"Renderer::ShadowMap", shadowMapDesc, D3D12_CLEAR_VALUE{DXGI_FORMAT_D24_UNORM_S8_UINT, 1, 0});
        m_ShadowMap.CreateShaderResourceView(m_ShadowMapSRV);
        m_ShadowMap.CreateDepthStencilView(m_ShadowMapDSV);
        m_ShadowMap.CreateDepthStencilView(m_ShadowMapDSVReadOnly, D3D12_DSV_FLAG_READ_ONLY_DEPTH);
        
        std::uint32_t destCount = 1;
        std::uint32_t srcCount[] = {1};
        D3D12_CPU_DESCRIPTOR_HANDLE srcTex[] = {
            m_ShadowMapSRV
        };
        g_RenderContext.GetDevice()->CopyDescriptors(
            1, &m_CommonTexture, &destCount, 1, srcTex, srcCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    
    Renderer::Renderer()
        :m_TextureHeap(L"Renderer::TexHeap", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, sm_MaxTextureSize),
        m_RootSignature(kNumRootBindings, 3),
        m_DefaultPSO(L"Renderer::DefaultPSO"),
        m_SkyboxPSO(L"Renderer::SkyboxPSO"){
    }

    void MeshSorter::AddMesh(const Mesh& mesh, float distance,
        D3D12_GPU_VIRTUAL_ADDRESS meshCBV,
        D3D12_GPU_VIRTUAL_ADDRESS matCBV)
    {
        SortKey key;
        key.m_Value = m_SortObjects.size();

        bool alphaBlend = (mesh.m_PSOFlags & kAlphaBlend) == kAlphaBlend;
        bool alphaTest = (mesh.m_PSOFlags & kAlphaTest) == kAlphaTest;
        // 从通用的PSO中获取，第一个为DepthOnly，第二个为AlphaTest
        std::uint64_t depthPSO = alphaTest ? 1 : 0;

        distance = Math::Max(distance, 0.0f);
        std::uint64_t disU = static_cast<std::uint64_t>(distance);

        if (m_BatchType == kShadows) {
            if (alphaBlend) return;

            key.m_PassId = kZPass;
            key.m_PSOIndex = depthPSO + 4;
            key.m_Key = disU;
            m_SortKey.emplace_back(key.m_Value);
            m_PassCounts[kZPass]++;
        }
        else if (alphaBlend) {
            key.m_PassId = kTransparent;
            key.m_PSOIndex = mesh.m_PSOIndex;
            // 透明物体远的优先渲染
            key.m_Key = ~disU;
            m_SortKey.emplace_back(key.m_Value);
            m_PassCounts[kTransparent]++;
        }
        else if (alphaTest || g_Renderer.m_SeparateZPass) {
            key.m_PassId = kZPass;
            key.m_PSOIndex = depthPSO;
            key.m_Key = disU;
            m_SortKey.emplace_back(key.m_Value);
            m_PassCounts[kZPass]++;

            key.m_PassId = kOpaque;
            key.m_PSOIndex = mesh.m_PSOIndex + 1;
            key.m_Key = disU;
            m_SortKey.emplace_back(key.m_Value);
            m_PassCounts[kOpaque]++;
        }
        else {
            key.m_PassId = kOpaque;
            key.m_PSOIndex = mesh.m_PSOIndex;
            key.m_Key = disU;
            m_SortKey.emplace_back(key.m_Value);
            m_PassCounts[kOpaque]++;
        }

        SortObject sortObject{&mesh, meshCBV, matCBV};
        m_SortObjects.emplace_back(std::move(sortObject));
    }

    void MeshSorter::Sort()
    {
        std::sort(m_SortKey.begin(), m_SortKey.end());
    }

    void MeshSorter::Render(DrawPass pass, GraphicsCommandList& cmdList, PassConstants& passConstants)
    {
        ASSERT(m_DepthTex != nullptr);

        auto pos = m_Camera->GetPosition();
        passConstants.m_CameraPos[0] = pos.GetX();
        passConstants.m_CameraPos[1] = pos.GetY();
        passConstants.m_CameraPos[2] = pos.GetZ();
        passConstants.m_Proj = Math::Matrix4::Transpose(m_Camera->GetProjMatrix());
        passConstants.m_View = Math::Matrix4::Transpose(m_Camera->GetViewMatrix());
        passConstants.m_ProjInv = Math::Matrix4::InverseTranspose(m_Camera->GetProjMatrix());
        passConstants.m_ViewInv = Math::Matrix4::InverseTranspose(m_Camera->GetViewMatrix());

        cmdList.TransitionResource(*m_DepthTex, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        cmdList.ClearDepth(m_DSV);

        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RTVs{};
        for (int i = 0; i < m_NumRTVs; ++i) {
            auto& renderTex = m_RenderTexs[i];
            cmdList.TransitionResource(*renderTex.m_RenderTex, D3D12_RESOURCE_STATE_RENDER_TARGET);
            cmdList.ClearRenderTarget(renderTex.m_RTV);
            RTVs.emplace_back(renderTex.m_RTV);
        }

        cmdList.SetRenderTargets(RTVs, m_DSV);
        cmdList.SetViewportAndScissor(m_Camera->GetViewPort(), m_Scissor);
    
        cmdList.SetRootSignature(g_Renderer.m_RootSignature);
        cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        cmdList.SetDescriptorHeap(g_Renderer.m_TextureHeap.GetHeap());
        cmdList.SetDynamicConstantBuffer(Renderer::kPassConstants, sizeof(PassConstants), &passConstants);
        cmdList.SetDescriptorTable(Renderer::kCommonSRVs, g_Renderer.m_CommonTexture);

        for (uint32_t currPass = kOpaque; currPass < kTransparent; ++currPass) {
            const uint32_t passCount = m_PassCounts[currPass];

            for (uint32_t currDraw = 0; currDraw < passCount; ++currDraw) {
                SortKey key{};
                key.m_Value = m_SortKey[currDraw];
                const SortObject& sortObject = m_SortObjects[key.m_ObjIndex];
                const Mesh* mesh = sortObject.m_Mesh;

                cmdList.SetConstantBuffer(Renderer::kMeshConstants, sortObject.m_MeshCBV);
                cmdList.SetConstantBuffer(Renderer::kMaterialConstants, sortObject.m_MaterialCBV);

                cmdList.SetPipelineState(g_Renderer.m_PSOs[key.m_PSOIndex]);

                std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferViews{};
                vertexBufferViews.emplace_back(mesh->m_PositionStream);
                if ((mesh->m_PSOFlags & kHasUV) != 0) {
                    vertexBufferViews.emplace_back(mesh->m_UVStream);
                }
                if ((mesh->m_PSOFlags & kHasNormal) != 0) {
                    vertexBufferViews.emplace_back(mesh->m_NormalStream);
                }
                if ((mesh->m_PSOFlags & kHasTangent) != 0) {
                    vertexBufferViews.emplace_back(mesh->m_TangentStream);
                }
                cmdList.SetVertexBuffers(0, vertexBufferViews);
                cmdList.SetIndexBuffer(mesh->m_IndexBufferViews);

                for (const auto& [name, submesh] : mesh->m_SubMeshes) {
                    cmdList.SetDescriptorTable(Renderer::kMaterialSRVs, g_Renderer.m_TextureHeap[submesh.m_SRVTableOffset]);
                    cmdList.DrawIndexed(submesh.m_IndexCount, submesh.m_IndexOffset, submesh.m_VertexOffset);
                }
            }
        }
    }
}
