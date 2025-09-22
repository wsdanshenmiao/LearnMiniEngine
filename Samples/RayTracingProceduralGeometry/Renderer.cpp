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
        m_LocalRootSig[0].InitAsConstants(1, sizeof(CubeConstantBuffer) / sizeof(uint32_t) + 1);
        m_LocalRootSig.Finalize(L"RayTracingLocalRootSignature", D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
        m_GlobalRootSig[RayTracingOutput].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);  // RayTracingOutput
        m_GlobalRootSig[AccelerationStructure].InitAsBufferSRV(0);  // 加速结构
        m_GlobalRootSig[SceneConstantBuffer].InitAsConstantBuffer(0);
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
        // 1 - Triangle hit group
        // 1 - Shader config
        // 2 - Local root signature and association
        // 1 - Global root signature
        // 1 - Pipeline config
        std::vector<D3D12_STATE_SUBOBJECT> subobjects;
        subobjects.reserve(7);

        ShaderDesc raytracingShaderDesc{};
        raytracingShaderDesc.m_Type = ShaderType::Lib;
        raytracingShaderDesc.m_Mode = ShaderMode::SM_6_6;
        raytracingShaderDesc.m_FileName = "Shaders//RayTracing.hlsl";
        ShaderByteCode shaderLib{raytracingShaderDesc};

        // DXIL library
        std::vector<D3D12_EXPORT_DESC> exportDescs(3);
        exportDescs[0].Name = s_RayGenShaderName;
        exportDescs[1].Name = s_MissShaderName;
        exportDescs[2].Name = s_ClosestHitShaderName;

        D3D12_DXIL_LIBRARY_DESC dxilLibDesc{};
        dxilLibDesc.DXILLibrary = shaderLib;
        dxilLibDesc.NumExports = static_cast<UINT>(exportDescs.size());
        dxilLibDesc.pExports = exportDescs.data();

        D3D12_STATE_SUBOBJECT libSubobject{};
        libSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        libSubobject.pDesc = &dxilLibDesc;
        subobjects.push_back(std::move(libSubobject));


        // Triangle hit group
        D3D12_HIT_GROUP_DESC hitGroupDesc{};
        hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
        hitGroupDesc.HitGroupExport = s_HitGroupName;
        hitGroupDesc.ClosestHitShaderImport = s_ClosestHitShaderName;

        D3D12_STATE_SUBOBJECT hitGroupSubobject{};
        hitGroupSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
        hitGroupSubobject.pDesc = &hitGroupDesc;
        subobjects.push_back(std::move(hitGroupSubobject));
    
    
        // Shader config
        D3D12_RAYTRACING_SHADER_CONFIG shaderConfig{};
        shaderConfig.MaxAttributeSizeInBytes = 2 * sizeof(float); // 三角形的重心坐标
        shaderConfig.MaxPayloadSizeInBytes = 4 * sizeof(float); // 光线的颜色

        D3D12_STATE_SUBOBJECT shaderConfigSubobject{};
        shaderConfigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
        shaderConfigSubobject.pDesc = &shaderConfig;
        subobjects.push_back(std::move(shaderConfigSubobject));


        // Local root signature and association
        D3D12_LOCAL_ROOT_SIGNATURE localRootSig{};
        localRootSig.pLocalRootSignature = m_LocalRootSig.GetRootSignature();

        D3D12_STATE_SUBOBJECT localRootSigSubobject{};
        localRootSigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
        localRootSigSubobject.pDesc = &localRootSig;
        auto& localSubobject = subobjects.emplace_back(std::move(localRootSigSubobject));

        // 将局部根签名与 shader 相关联
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localRootSigAssociation{};
        localRootSigAssociation.pSubobjectToAssociate = &localSubobject;
        localRootSigAssociation.NumExports = 1;
        localRootSigAssociation.pExports = &s_HitGroupName; // Hit Group 的导出名
        D3D12_STATE_SUBOBJECT localRootSigAssociationSubobject{};
        localRootSigAssociationSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        localRootSigAssociationSubobject.pDesc = &localRootSigAssociation;
        subobjects.push_back(std::move(localRootSigAssociationSubobject));


        // Global root signature
        D3D12_GLOBAL_ROOT_SIGNATURE globalRootSig{};
        globalRootSig.pGlobalRootSignature = m_GlobalRootSig.GetRootSignature();

        D3D12_STATE_SUBOBJECT globalRootSigSubobject{};
        globalRootSigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
        globalRootSigSubobject.pDesc = &globalRootSig;
        subobjects.push_back(std::move(globalRootSigSubobject));


        // Pipeline config
        D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig{};
        pipelineConfig.MaxTraceRecursionDepth = 1;  // 最大递归深度

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
        m_LocalRootSig(1, 0),
        m_GlobalRootSig(Count, 0) {}




    
    void RayTracer::TraceRays(ComputeCommandList &cmdList)
    {
        ASSERT(m_Camera != nullptr);

        if (m_Models.empty()) 
            return;

        auto& imgui = ImguiManager::GetInstance();

        uint32_t width = m_Camera->GetViewPort().Width;
        uint32_t height = m_Camera->GetViewPort().Height;

        cmdList.SetRootSignature(g_Renderer.m_GlobalRootSig);
        cmdList.SetDescriptorHeap(g_Renderer.m_TextureHeap.GetHeap());

        float focusDist = 10;
        auto h = std::tan(m_Camera->GetFovY() * .5f);
        auto viewportHeight = 2 * h * focusDist;
        float viewportWidth = viewportHeight * (float(width) / height);
        SceneConstantBuffer sceneCB{};
        sceneCB.cameraPosAndFocusDist = Math::Vector4{m_Camera->GetPosition(), focusDist};
        sceneCB.viewportU = Math::Vector4{m_Camera->GetRightAxis() * viewportWidth};
        sceneCB.viewportV = Math::Vector4{-m_Camera->GetUpAxis() * viewportHeight};
        sceneCB.lightColor = Math::Vector4{imgui.lightColor, 0};
        sceneCB.lightDirAndGloss = Math::Vector4{imgui.lightDir.Normalized(), float(imgui.cubeGloss)};

        CubeConstantBuffer cubeCB{};
        cubeCB.albedo = Math::Vector4{imgui.cubeAlbedo};
        m_HitShaderTable.Update(&cubeCB, sizeof(CubeConstantBuffer), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

        cmdList.SetDescriptorTable(Renderer::RayTracingOutput, g_Renderer.m_OutputUAV);
        cmdList.SetShaderResource(Renderer::AccelerationStructure, m_TopLevelAS.accelerationStructure);
        cmdList.SetDynamicConstantBuffer(Renderer::SceneConstantBuffer, sizeof(SceneConstantBuffer), &sceneCB);

        D3D12_DISPATCH_RAYS_DESC dispatchDesc{};
        dispatchDesc.HitGroupTable.StartAddress = m_HitShaderTable->GetGPUVirtualAddress();
        dispatchDesc.HitGroupTable.SizeInBytes = m_HitShaderTable.GetSize();
        dispatchDesc.HitGroupTable.StrideInBytes = dispatchDesc.HitGroupTable.SizeInBytes;
        dispatchDesc.MissShaderTable.StartAddress = m_MissShaderTable->GetGPUVirtualAddress();
        dispatchDesc.MissShaderTable.SizeInBytes = m_MissShaderTable.GetSize();
        dispatchDesc.MissShaderTable.StrideInBytes = dispatchDesc.MissShaderTable.SizeInBytes;
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

    void RayTracer::CreateAccelerationStructure()
    {
        // 构建加速结构
        GraphicsCommandList cmdList{L"BuildAccelerationStructure"};

        const auto& model = m_Models[0];
        const auto& mesh = model->meshes[0];
        const auto& submesh = mesh->m_SubMeshes.begin()->second;

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
                    instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 0.01f;
                    instanceDesc.InstanceMask = 1;
                    instanceDesc.InstanceContributionToHitGroupIndex = 0;
                    instanceDesc.AccelerationStructure = bottomLevelBuffers.accelerationStructure.GetGpuVirtualAddress();
                    instanceDescs.push_back(std::move(instanceDesc));
                }
            }
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
        void* missShaderIdentifier = stateObjectProps->GetShaderIdentifier(Renderer::s_MissShaderName);
        void* hitGroupIdentifier = stateObjectProps->GetShaderIdentifier(Renderer::s_HitGroupName);

        // RayGeneration 着色器表
        GpuBufferDesc rayGenShaderTableDesc{};
        rayGenShaderTableDesc.m_Size = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        rayGenShaderTableDesc.m_Stride = rayGenShaderTableDesc.m_Size;
        rayGenShaderTableDesc.m_HeapType = D3D12_HEAP_TYPE_UPLOAD;
        m_RayGenShaderTable.Create(L"RayGenShaderTable", rayGenShaderTableDesc, rayGenShaderIdentifier);
        // Miss 着色器表
        GpuBufferDesc missShaderTableDesc = rayGenShaderTableDesc;
        missShaderTableDesc.m_Size = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        missShaderTableDesc.m_Stride = missShaderTableDesc.m_Size;
        m_MissShaderTable.Create(L"MissShaderTable", missShaderTableDesc, missShaderIdentifier);
        // Hit 着色器表
        GpuBufferDesc hitShaderTableDesc = rayGenShaderTableDesc;
        auto size = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + Math::AlignUp(sizeof(CubeConstantBuffer), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        hitShaderTableDesc.m_Size = Math::AlignUp(size, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
        hitShaderTableDesc.m_Stride = hitShaderTableDesc.m_Size;
        std::vector<uint8_t> hitShaderTableData(hitShaderTableDesc.m_Size);
        memcpy(hitShaderTableData.data(), hitGroupIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        m_HitShaderTable.Create(L"HitShaderTable", hitShaderTableDesc, hitShaderTableData.data());
    }


}