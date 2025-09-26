#include "Renderer.h"
#include "Graphics/RenderContext.h"
#include "Graphics/CommandList/GraphicsCommandList.h"
#include "Geometry.h"
#include "Graphics/CommandList/ComputeCommandList.h"
#include "ImguiManager.h"
#include "Model.h"
#include "Material.h"

namespace DSM {

    // Renderer implementation
    void Renderer::Create()
    {
        if(m_Initialized) return;

        auto width = g_RenderContext.GetSwapChain().GetWidth();
        auto height = g_RenderContext.GetSwapChain().GetHeight();

        m_OutputUAV = m_TextureHeap.Allocate(1);
        CreateResource(width, height);
        
        // 创建根签名
        // 给 HitGroup 设置的资源
        auto& triangleRootSig = m_LocalRootSigs[LocalRootSignature::Type::Triangle];
        auto& aabbRootSig = m_LocalRootSigs[LocalRootSignature::Type::AABB];
        triangleRootSig[LocalRootSignature::Triangle::Slot::Material].InitAsConstants(1, Math::AlignUp(sizeof(MaterialConstantBuffer), 4) / sizeof(uint32_t));
        triangleRootSig[LocalRootSignature::Triangle::Slot::IndexBuffer].InitAsBufferSRV(1);
        triangleRootSig[LocalRootSignature::Triangle::Slot::NormalBuffer].InitAsBufferSRV(2);
        triangleRootSig[LocalRootSignature::Triangle::Slot::UVBuffer].InitAsBufferSRV(3);
        triangleRootSig[LocalRootSignature::Triangle::Slot::Textures].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, kNumTextures);
        triangleRootSig.Finalize(L"RayTracingLocalRootSignature_Triangle", D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
        
        size_t instanceCBSize = Math::AlignUp(sizeof(RayTracing::PrimitiveInstanceConstantBuffer), 4);
        aabbRootSig[LocalRootSignature::AABB::Slot::PrimitiveInstance].InitAsConstants(2, instanceCBSize / sizeof(uint32_t));
        aabbRootSig[LocalRootSignature::AABB::Slot::Material].InitAsConstantBuffer(1);
        aabbRootSig[LocalRootSignature::AABB::Slot::Textures].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, kNumTextures);
        aabbRootSig.Finalize(L"RayTracingLocalRootSignature_AABB", D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

        m_GlobalRootSig[GlobalRootSignature::RayTracing::RayTracingOutput].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);  // RayTracingOutput
        m_GlobalRootSig[GlobalRootSignature::RayTracing::AccelerationStructure].InitAsBufferSRV(0);  // 加速结构
        m_GlobalRootSig[GlobalRootSignature::RayTracing::SceneConstantBuffer].InitAsConstantBuffer(0);
        m_GlobalRootSig[GlobalRootSignature::Light::LightData].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_ALL, 1);
        m_GlobalRootSig[GlobalRootSignature::Light::DirectionalLightDatas].InitAsBufferSRV(0, D3D12_SHADER_VISIBILITY_ALL, 1);
        m_GlobalRootSig.InitStaticSampler(GlobalRootSignature::StaticSampler::AnisoWrap, Graphics::SamplerAnisoWrap);
        m_GlobalRootSig.Finalize(L"RayTracingGlobalRootSignature");

        CreateStateObject();

        m_Initialized = true;
    }

    void Renderer::Shutdown()
    {
        m_Initialized = false;
    }

    void Renderer::OnResize(uint32_t width, uint32_t height)
    {
        CreateResource(width, height);
    }

    void Renderer::CreateResource(uint32_t width, uint32_t height)
    {
        // 创建几何数据
        TextureDesc texDesc{};
        texDesc.m_Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.m_Width = width;
        texDesc.m_Height = height;
        texDesc.m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        m_RayTracingOutput.Create(L"RayTracingOutput", texDesc, {}, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_RayTracingOutput.CreateUnorderedAccessView(m_OutputUAV);
    }

    void DSM::Renderer::CreateStateObject()
    {
        // 创建光线追踪管线
        // 创建一个光线追踪管线状态对象需要又七个子对象
        // 每个子对象都需要关联到着色器
        // 1 - DXIL library
        // 2 - Triangle hit group
        // 2 - AABB hit group
        // 2 - Analytic primitive hit group
        // 1 - Shader config
        // 2 - Local root signature and association
        // 1 - Global root signature
        // 1 - Pipeline config
        std::vector<D3D12_STATE_SUBOBJECT> subobjects;
        subobjects.reserve(12); // 注意要预留足够的空间，避免内存重新分配造成空引用

        ShaderDesc raytracingShaderDesc{};
        raytracingShaderDesc.m_Type = ShaderType::Lib;
        raytracingShaderDesc.m_Mode = ShaderMode::SM_6_6;
        raytracingShaderDesc.m_FileName = "Shaders//RayTracing.hlsl";
        ShaderByteCode shaderLib{raytracingShaderDesc};

        // DXIL library
        D3D12_DXIL_LIBRARY_DESC dxilLibDesc{};  // 使用默认导出名
        dxilLibDesc.DXILLibrary = shaderLib;

        D3D12_STATE_SUBOBJECT libSubobject{};
        libSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        libSubobject.pDesc = &dxilLibDesc;
        subobjects.push_back(std::move(libSubobject));


        // Hit group
        // 由于需要取地址，所以需要额外保存
        std::array<D3D12_HIT_GROUP_DESC, RayTracing::RayType::Count> hitGroupDescsTriangle{};
        for(int i = 0; i < RayTracing::RayType::Count; ++i){
            auto& hitGroupDesc = hitGroupDescsTriangle[i];
            hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
            hitGroupDesc.HitGroupExport = s_HitGroupName_Triangle[i];
            hitGroupDesc.ClosestHitShaderImport = s_ClosestHitShaderName[GeometryType::Triangle];

            D3D12_STATE_SUBOBJECT hitGroupSubobject{};
            hitGroupSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            hitGroupSubobject.pDesc = &hitGroupDesc;
            subobjects.push_back(std::move(hitGroupSubobject));
        }
        std::array<D3D12_HIT_GROUP_DESC, RayTracing::RayType::Count> hitGroupDescsAABB{};
        for (int i = 0; i < RayTracing::RayType::Count; ++i) {
            auto& hitGroupDesc = hitGroupDescsAABB[i];
            hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
            hitGroupDesc.HitGroupExport = s_HitGroupName_AABB[i];
            hitGroupDesc.ClosestHitShaderImport = s_ClosestHitShaderName[GeometryType::AABB];
            hitGroupDesc.IntersectionShaderImport = s_IntersectionShaderName[IntersectionShaderType::AnalyticPrimitive];

            D3D12_STATE_SUBOBJECT hitGroupSubobject{};
            hitGroupSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            hitGroupSubobject.pDesc = &hitGroupDesc;
            subobjects.push_back(std::move(hitGroupSubobject));
        }


        // Shader config
        D3D12_RAYTRACING_SHADER_CONFIG shaderConfig{};
        shaderConfig.MaxPayloadSizeInBytes = (std::max)(sizeof(RayTracing::RayPayload), sizeof(RayTracing::ShadowRayPayload)); // 光线的颜色
        shaderConfig.MaxAttributeSizeInBytes = sizeof(RayTracing::ProceduralPrimitiveAttributes); // 三角形的重心坐标

        D3D12_STATE_SUBOBJECT shaderConfigSubobject{};
        shaderConfigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
        shaderConfigSubobject.pDesc = &shaderConfig;
        subobjects.push_back(std::move(shaderConfigSubobject));


        // Local root signature and association
        std::array<D3D12_LOCAL_ROOT_SIGNATURE, LocalRootSignature::Type::Count> localRootSigs{};
        for(int i = 0; i < LocalRootSignature::Type::Count; ++i){
            localRootSigs[i].pLocalRootSignature = m_LocalRootSigs[i].GetRootSignature();
        }

        std::array<const D3D12_STATE_SUBOBJECT*, LocalRootSignature::Type::Count> localSubobjects{};
        for(int i = 0; i < LocalRootSignature::Type::Count; ++i){
            D3D12_STATE_SUBOBJECT localRootSigSubobject{};
            localRootSigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
            localRootSigSubobject.pDesc = &localRootSigs[i];
            localSubobjects[i] = &subobjects.emplace_back(std::move(localRootSigSubobject));
        }

        // 关联三角形的根签名
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localRootSigAssociationTriangle{};
        localRootSigAssociationTriangle.pSubobjectToAssociate = localSubobjects[LocalRootSignature::Type::Triangle];
        localRootSigAssociationTriangle.NumExports = 1;
        localRootSigAssociationTriangle.pExports = &s_HitGroupName_Triangle[RayTracing::RayType::Radiance];
        D3D12_STATE_SUBOBJECT localRootSigAssociationSubobject{};
        localRootSigAssociationSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        localRootSigAssociationSubobject.pDesc = &localRootSigAssociationTriangle;
        subobjects.push_back(std::move(localRootSigAssociationSubobject));

        // 关联程序图元的根签名
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localRootSigAssociationProcedural{};
        localRootSigAssociationProcedural.pSubobjectToAssociate = localSubobjects[LocalRootSignature::Type::AABB];
        localRootSigAssociationProcedural.NumExports = 1;
        localRootSigAssociationProcedural.pExports = &s_HitGroupName_AABB[RayTracing::RayType::Radiance];
        D3D12_STATE_SUBOBJECT localRootSigAssociationSubobjectProcedural{};
        localRootSigAssociationSubobjectProcedural.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        localRootSigAssociationSubobjectProcedural.pDesc = &localRootSigAssociationProcedural;
        subobjects.push_back(std::move(localRootSigAssociationSubobjectProcedural));

        // Global root signature
        D3D12_GLOBAL_ROOT_SIGNATURE globalRootSig{};
        globalRootSig.pGlobalRootSignature = m_GlobalRootSig.GetRootSignature();

        D3D12_STATE_SUBOBJECT globalRootSigSubobject{};
        globalRootSigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
        globalRootSigSubobject.pDesc = &globalRootSig;
        subobjects.push_back(std::move(globalRootSigSubobject));


        // Pipeline config
        D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig{};
        pipelineConfig.MaxTraceRecursionDepth = s_MaxTraceRecursionDepth;  // 最大递归深度

        D3D12_STATE_SUBOBJECT pipelineConfigSubobject{};
        pipelineConfigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
        pipelineConfigSubobject.pDesc = &pipelineConfig;
        subobjects.push_back(std::move(pipelineConfigSubobject));


        // 创建管线状态对象
        D3D12_STATE_OBJECT_DESC stateObjectDesc{};
        stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
        stateObjectDesc.NumSubobjects = static_cast<UINT>(subobjects.size());
        stateObjectDesc.pSubobjects = subobjects.data();
        g_RenderContext.GetDevice()->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(m_RayTracingStateObject.GetAddressOf()));
    }

    Renderer::Renderer()
        :m_TextureHeap(L"Renderer::TextureHeap", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096),
        m_LocalRootSigs({{LocalRootSignature::Triangle::Slot::Count, 0}, 
            {LocalRootSignature::AABB::Slot::Count, 0}}),
        m_GlobalRootSig(GlobalRootSignature::GlobalRootSignatureCount, 
            GlobalRootSignature::StaticSampler::Count) {}





    RayTracer::RayTracer()
        :m_ProceduralGeometryManager(std::make_unique<ProceduralGeometryManager>())
    {
        m_DirLights.emplace_back();

        GpuBufferDesc lightDataBufferDesc{};
        lightDataBufferDesc.m_Size = Math::AlignUp(sizeof(LightData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        lightDataBufferDesc.m_Stride = lightDataBufferDesc.m_Size;
        m_LightDataBuffer.Create(L"RayTracer::LightDataBuffer", lightDataBufferDesc);

        GpuBufferDesc dirLightDataBufferDesc{};
        dirLightDataBufferDesc.m_Size = sizeof(DirectionalLightData) * sm_MaxDirLightCount;
        dirLightDataBufferDesc.m_Stride = sizeof(DirectionalLightData);
        m_DirLightDataBuffer.Create(L"RayTracer::DirLightDataBuffer", dirLightDataBufferDesc);
    }
    
    void RayTracer::TraceRays(ComputeCommandList &cmdList)
    {
        ASSERT(m_Camera != nullptr);

        if (m_Models.empty()) 
            return;

        auto& imgui = ImguiManager::GetInstance();
        m_DirLights[0].color = Math::Vector4{imgui.lightColor, 0};
        m_DirLights[0].direction = -Math::Vector4{imgui.lightDir.Normalized(), 0};

        if(!m_DirLights.empty()){
            static uint32_t dirLightCount = 0;
            if(dirLightCount != m_DirLights.size()){
                LightData lightData{m_DirLights.size()};
                cmdList.WriteBuffer(m_LightDataBuffer, 0, &lightData, sizeof(LightData));
                cmdList.WriteBuffer(m_DirLightDataBuffer, 0, m_DirLights.data(), m_DirLights.size() * sizeof(DirectionalLightData));
            }
        }

        uint32_t width = m_Camera->GetViewPort().Width;
        uint32_t height = m_Camera->GetViewPort().Height;

        cmdList.SetRootSignature(g_Renderer.m_GlobalRootSig);
        cmdList.SetDescriptorHeap(g_Renderer.m_TextureHeap.GetHeap());

        float focusDist = 10;
        auto h = std::tan(m_Camera->GetFovY() * .5f);
        auto viewportHeight = 2 * h * focusDist;
        float viewportWidth = viewportHeight * (float(width) / height);
        RayTracing::SceneConstantBuffer sceneCB{};
        sceneCB.cameraPosAndFocusDist = Math::Vector4{m_Camera->GetPosition(), focusDist};
        sceneCB.viewportU = Math::Vector4{m_Camera->GetRightAxis() * viewportWidth};
        sceneCB.viewportV = Math::Vector4{-m_Camera->GetUpAxis() * viewportHeight};

        cmdList.SetDescriptorTable(GlobalRootSignature::RayTracing::RayTracingOutput, g_Renderer.m_OutputUAV);
        cmdList.SetShaderResource(GlobalRootSignature::RayTracing::AccelerationStructure, m_TopLevelAS.accelerationStructure);
        cmdList.SetDynamicConstantBuffer(GlobalRootSignature::RayTracing::SceneConstantBuffer, sizeof(RayTracing::SceneConstantBuffer), &sceneCB);
        cmdList.SetConstantBuffer(GlobalRootSignature::Light::LightData, m_LightDataBuffer.GetGpuVirtualAddress());
        cmdList.SetShaderResource(GlobalRootSignature::Light::DirectionalLightDatas, m_DirLightDataBuffer);

        D3D12_DISPATCH_RAYS_DESC dispatchDesc{};
        dispatchDesc.HitGroupTable.StartAddress = m_HitShaderTable->GetGPUVirtualAddress();
        dispatchDesc.HitGroupTable.SizeInBytes = m_HitShaderTable.GetSize();
        dispatchDesc.HitGroupTable.StrideInBytes = m_HitShaderTable.GetStride();
        dispatchDesc.MissShaderTable.StartAddress = m_MissShaderTable->GetGPUVirtualAddress();
        dispatchDesc.MissShaderTable.SizeInBytes = m_MissShaderTable.GetSize();
        dispatchDesc.MissShaderTable.StrideInBytes = m_MissShaderTable.GetStride();
        dispatchDesc.RayGenerationShaderRecord.StartAddress = m_RayGenShaderTable->GetGPUVirtualAddress();
        dispatchDesc.RayGenerationShaderRecord.SizeInBytes = m_RayGenShaderTable.GetSize();
        dispatchDesc.Width = width;
        dispatchDesc.Height = height;
        dispatchDesc.Depth = 1;
        cmdList.GetDXRCommandList()->SetPipelineState1(g_Renderer.m_RayTracingStateObject.Get());
        cmdList.GetDXRCommandList()->DispatchRays(&dispatchDesc);
    }

    void RayTracer::AddModel(std::shared_ptr<Model> model)
    {
        m_Models.push_back(model);
        CreateAccelerationStructure();
        CreateShaderTable();
    }

    void RayTracer::AddProceduralGeometry(const ProceduralGeometryDesc &desc)
    {
        m_ProceduralGeometryManager->AddGeometry(desc);
        CreateAccelerationStructure();
        CreateShaderTable();
    }

    void RayTracer::AddLight(const Light &light)
    {
        switch (light.lightType) {
        case LightType::Directional:
            m_DirLights.emplace_back(light.color, -Math::Vector4{light.transform.GetForwardAxis()}); break;
        default:
            break;
        }
    }

    void RayTracer::CreateAccelerationStructure()
    {
        // 构建加速结构
        GraphicsCommandList cmdList{L"BuildAccelerationStructure"};

        const auto& model = m_Models[0];
        const auto& mesh = model->meshes[0];
        const auto& submesh = mesh->m_SubMeshes.begin()->second;

        uint32_t instanceContributionToHitGroupIndex = 0;
        std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
        for(const auto& model : m_Models){
            for(const auto& mesh : model->meshes){
                for(const auto& [name, submesh] : mesh->m_SubMeshes){
                    auto& bottomLevelBuffers = m_BottomLevelASs.emplace_back();

                    // 给底层加速结构的几何描述
                    D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC trianglesDesc{};
                    trianglesDesc.Transform3x4 = 0;
                    trianglesDesc.IndexFormat = DXGI_FORMAT_R32_UINT;
                    trianglesDesc.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
                    trianglesDesc.IndexCount = submesh.m_IndexCount;
                    trianglesDesc.VertexCount = submesh.m_VertexCount;
                    trianglesDesc.IndexBuffer = mesh->m_IndexBufferViews.BufferLocation + submesh.m_IndexOffset * sizeof(uint32_t);
                    trianglesDesc.VertexBuffer.StartAddress = mesh->m_PositionStream.BufferLocation + submesh.m_VertexOffset * mesh->m_PositionStream.StrideInBytes;
                    trianglesDesc.VertexBuffer.StrideInBytes = mesh->m_PositionStream.StrideInBytes;
                    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc{};
                    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
                    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
                    geometryDesc.Triangles = trianglesDesc;

                    // 底层加速结构的输入
                    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelASInputs{};
                    bottomLevelASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
                    bottomLevelASInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
                    bottomLevelASInputs.NumDescs = 1;
                    bottomLevelASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
                    bottomLevelASInputs.pGeometryDescs = &geometryDesc;

                    // 获取加速结构的相关信息
                    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelASInfo{};
                    g_RenderContext.GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelASInputs, &bottomLevelASInfo);
                    ASSERT(bottomLevelASInfo.ResultDataMaxSizeInBytes > 0);

                    // 为加速结构分配显存
                    GpuBufferDesc bottomLevelASDesc{};
                    bottomLevelASDesc.m_Size = bottomLevelASInfo.ResultDataMaxSizeInBytes;
                    bottomLevelASDesc.m_Stride = bottomLevelASDesc.m_Size;
                    bottomLevelASDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                    bottomLevelASDesc.m_ResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
                    bottomLevelBuffers.accelerationStructure.Create(L"BottomLevelAS", bottomLevelASDesc);

                    uint64_t bottomLevelScratchBufferSize = (std::max)(bottomLevelASInfo.UpdateScratchDataSizeInBytes, bottomLevelASInfo.ScratchDataSizeInBytes);
                    GpuBufferDesc bottomLevelScratchBufferDesc{};
                    bottomLevelScratchBufferDesc.m_Size = bottomLevelScratchBufferSize;
                    bottomLevelScratchBufferDesc.m_Stride = bottomLevelScratchBufferDesc.m_Size;
                    bottomLevelScratchBufferDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                    bottomLevelBuffers.scratch.Create(L"BottomLevelASScratch", bottomLevelScratchBufferDesc);

                    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildBottomLevelASDesc{};
                    buildBottomLevelASDesc.Inputs = bottomLevelASInputs;
                    buildBottomLevelASDesc.ScratchAccelerationStructureData = bottomLevelBuffers.scratch.GetGpuVirtualAddress();
                    buildBottomLevelASDesc.DestAccelerationStructureData = bottomLevelBuffers.accelerationStructure.GetGpuVirtualAddress();

                    cmdList.GetDXRCommandList()->BuildRaytracingAccelerationStructure(&buildBottomLevelASDesc, 0, nullptr);
                    // 等待底层加速结构构建完毕
                    cmdList.InsertUAVBarrier(bottomLevelBuffers.accelerationStructure, true);

                    // 顶层加速结构的输入，使用底层加速结构作为输入
                    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc{};
                    instanceDesc.InstanceMask = 1;
                    instanceDesc.InstanceContributionToHitGroupIndex = instanceContributionToHitGroupIndex;
                    instanceDesc.AccelerationStructure = bottomLevelBuffers.accelerationStructure.GetGpuVirtualAddress();
                    DirectX::XMStoreFloat3x4(&reinterpret_cast<DirectX::XMFLOAT3X4&>(instanceDesc.Transform), model->transform.GetLocalToWorld());
                    instanceDescs.push_back(std::move(instanceDesc));
                    instanceContributionToHitGroupIndex += bottomLevelASInputs.NumDescs * RayTracing::RayType::Count;
                }
            }
        }

        // 添加程序图元的实例
        for(const auto& geometry : m_ProceduralGeometryManager->GetAllGeometry()){
            auto instanceDesc = geometry.instanceDesc;
            instanceDesc.InstanceContributionToHitGroupIndex += instanceContributionToHitGroupIndex;
            instanceDescs.push_back(std::move(instanceDesc));
        }
        
        GpuBufferDesc instanceBufferDesc{};
        instanceBufferDesc.m_Size = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDescs.size();
        instanceBufferDesc.m_Stride = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
        m_TopLevelAS.instanceDesc.Create(L"TopLevelASInstanceDesc", instanceBufferDesc, instanceDescs.data());

        // 顶层加速结构的输入
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelASInputs{};
        topLevelASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        topLevelASInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        topLevelASInputs.NumDescs = instanceDescs.size();
        topLevelASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        topLevelASInputs.InstanceDescs = m_TopLevelAS.instanceDesc.GetGpuVirtualAddress();

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelASInfo{};
        g_RenderContext.GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelASInputs, &topLevelASInfo);
        ASSERT(topLevelASInfo.ResultDataMaxSizeInBytes > 0);

        GpuBufferDesc topLevelASDesc{};
        topLevelASDesc.m_Size = topLevelASInfo.ResultDataMaxSizeInBytes;
        topLevelASDesc.m_Stride = topLevelASDesc.m_Size;
        topLevelASDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        topLevelASDesc.m_ResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        m_TopLevelAS.accelerationStructure.Create(L"TopLevelAS", topLevelASDesc);

        // 分配加速结构生成需要的暂存空间
        uint64_t topLevelScratchBufferSize = (std::max)(topLevelASInfo.UpdateScratchDataSizeInBytes, topLevelASInfo.ScratchDataSizeInBytes);
        GpuBufferDesc topLevelScratchBufferDesc{};
        topLevelScratchBufferDesc.m_Size = topLevelScratchBufferSize;
        topLevelScratchBufferDesc.m_Stride = topLevelScratchBufferSize;
        topLevelScratchBufferDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        m_TopLevelAS.scratch.Create(L"ScratchBuffer", topLevelScratchBufferDesc);

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildTopLevelASDesc{};
        buildTopLevelASDesc.Inputs = topLevelASInputs;
        buildTopLevelASDesc.ScratchAccelerationStructureData = m_TopLevelAS.scratch.GetGpuVirtualAddress();
        buildTopLevelASDesc.DestAccelerationStructureData = m_TopLevelAS.accelerationStructure.GetGpuVirtualAddress();

        cmdList.GetDXRCommandList()->BuildRaytracingAccelerationStructure(&buildTopLevelASDesc, 0, nullptr);
        cmdList.ExecuteCommandList(true);
    }

    void RayTracer::CreateShaderTable()
    {
        // 创建着色器表
        // 获取 Shader 的标识符
        Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> stateObjectProps{};
        ASSERT_SUCCEEDED(g_Renderer.m_RayTracingStateObject.As(&stateObjectProps));
        void* rayGenShaderIdentifier = stateObjectProps->GetShaderIdentifier(Renderer::s_RayGenShaderName);
        std::array<void*, RayTracing::RayType::Count> missShaderIdentifiers{};
        for (size_t i = 0; i < RayTracing::RayType::Count; i++) {
            missShaderIdentifiers[i] = stateObjectProps->GetShaderIdentifier(Renderer::s_MissShaderName[i]);
        }
        std::array<void*, RayTracing::RayType::Count> hitGroupIdentifiersTriangle{};
        for (size_t i = 0; i < RayTracing::RayType::Count; i++) {
            hitGroupIdentifiersTriangle[i] = stateObjectProps->GetShaderIdentifier(Renderer::s_HitGroupName_Triangle[i]);
        }
        std::array<void*, RayTracing::RayType::Count> hitGroupIdentifiersAABB{};
        for (size_t i = 0; i < RayTracing::RayType::Count; i++) {
            hitGroupIdentifiersAABB[i] = stateObjectProps->GetShaderIdentifier(Renderer::s_HitGroupName_AABB[i]);
        }

        constexpr uint32_t shaderIdSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

        // RayGeneration 着色器表
        GpuBufferDesc rayGenShaderTableDesc{};
        rayGenShaderTableDesc.m_Size = shaderIdSize;
        rayGenShaderTableDesc.m_Stride = rayGenShaderTableDesc.m_Size;
        rayGenShaderTableDesc.m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
        m_RayGenShaderTable.Create(L"RayGenShaderTable", rayGenShaderTableDesc, rayGenShaderIdentifier);

        // Miss 着色器表
        constexpr uint32_t missShaderTableSize = shaderIdSize * RayTracing::RayType::Count;
        std::array<uint8_t, missShaderTableSize> missShaderTableData{};
        for(int i = 0; i < RayTracing::RayType::Count; ++i){
            memcpy(missShaderTableData.data() + i * shaderIdSize, missShaderIdentifiers[i], shaderIdSize);
        }
        GpuBufferDesc missShaderTableDesc = rayGenShaderTableDesc;
        missShaderTableDesc.m_Size = missShaderTableSize;
        missShaderTableDesc.m_Stride = shaderIdSize;
        m_MissShaderTable.Create(L"MissShaderTable", missShaderTableDesc, missShaderTableData.data());

        // Hit 着色器表
        uint32_t hitGroupShaderRecordSize = shaderIdSize + LocalRootSignature::MaxRootArgumentsSize();
        hitGroupShaderRecordSize = Math::AlignUp(hitGroupShaderRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
        std::vector<uint8_t> hitGroupShaderTableData{};
        std::vector<uint8_t> hitGroupShaderRecordData{};
        for (const auto& model : m_Models){
            for(const auto& mesh : model->meshes){
                for(const auto& [name, submesh] : mesh->m_SubMeshes) {
                    for(int i = 0; i < RayTracing::RayType::Count; i++) {
                        hitGroupShaderRecordData.clear();
                        // 全部清零以免残余数据影响
                        hitGroupShaderRecordData.resize(hitGroupShaderRecordSize, 0);
                        memcpy(hitGroupShaderRecordData.data(), hitGroupIdentifiersTriangle[i], shaderIdSize);
                        if(i == RayTracing::RayType::Radiance){ // 只有渲染光线需要填入根参数
                            LocalRootSignature::Triangle::RootArguments rootArgs{};
                            auto meshMat = model->materials[submesh.m_MaterialIndex];
                            rootArgs.material.baseColor = meshMat->baseColor;
                            rootArgs.material.emissiveColor = meshMat->emissiveColor;
                            rootArgs.material.metallicFactor = meshMat->metallicFactor;
                            rootArgs.material.roughnessFactor = meshMat->roughnessFactor;
                            rootArgs.material.normalTexScale = meshMat->normalTexScale;
                            rootArgs.indexBuffer = mesh->m_IndexBufferViews.BufferLocation + submesh.m_IndexOffset * sizeof(uint32_t);
                            rootArgs.normalBuffer = mesh->m_NormalStream.BufferLocation + 
                                submesh.m_VertexOffset * mesh->m_NormalStream.StrideInBytes;
                            rootArgs.uvBuffer = mesh->m_UVStream.BufferLocation + 
                                submesh.m_VertexOffset * mesh->m_UVStream.StrideInBytes;
                            rootArgs.textures = g_Renderer.m_TextureHeap[submesh.m_SRVTableOffset];
                            memcpy(hitGroupShaderRecordData.data() + shaderIdSize, &rootArgs, sizeof(rootArgs));
                        }
                        hitGroupShaderTableData.append_range(hitGroupShaderRecordData);
                    }
                }
            }
        }

        for(const auto& geometry : m_ProceduralGeometryManager->GetAllGeometry()){
            for(int i = 0; i < RayTracing::RayType::Count; ++i){
                hitGroupShaderRecordData.clear();
                hitGroupShaderRecordData.resize(hitGroupShaderRecordSize, 0);
                memcpy(hitGroupShaderRecordData.data(), hitGroupIdentifiersAABB[i], shaderIdSize);
                LocalRootSignature::AABB::RootArguments rootArgs{};
                rootArgs.primitiveInstance.primitiveType = geometry.type;
                rootArgs.material = geometry.materialBuffer.GetGpuVirtualAddress();
                rootArgs.textures = g_Renderer.m_TextureHeap[geometry.srvOffset];
                memcpy(hitGroupShaderRecordData.data() + shaderIdSize, &rootArgs, sizeof(rootArgs));
                hitGroupShaderTableData.append_range(hitGroupShaderRecordData);
            }
        }

        GpuBufferDesc hitShaderTableDesc = rayGenShaderTableDesc;
        hitShaderTableDesc.m_Size = Math::AlignUp(hitGroupShaderTableData.size(), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
        hitShaderTableDesc.m_Stride = hitGroupShaderRecordSize;
        hitGroupShaderTableData.resize(hitShaderTableDesc.m_Size);
        m_HitShaderTable.Create(L"HitShaderTable", hitShaderTableDesc, hitGroupShaderTableData.data());
    }


}