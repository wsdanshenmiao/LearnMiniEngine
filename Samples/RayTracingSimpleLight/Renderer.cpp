#include "Renderer.h"
#include "Graphics/RenderContext.h"
#include "Graphics/CommandList/GraphicsCommandList.h"
#include "Geometry.h"
#include "Graphics/CommandList/ComputeCommandList.h"
#include "ImguiManager.h"

namespace DSM {

    // Renderer implementation
    void Renderer::Create()
    {
        if(m_Initialized) return;

        auto mesh = Geometry::GeometryGenerator::CreateBox(3, 3, 3, 0);

        GpuBufferDesc vertexBufferDesc{};
        vertexBufferDesc.m_Size = sizeof(Geometry::Vertex) * mesh.m_Vertices.size();
        vertexBufferDesc.m_Stride = sizeof(Geometry::Vertex);
        vertexBufferDesc.m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
        m_VertexBuffer.Create(L"VertexBuffer", vertexBufferDesc, mesh.m_Vertices.data());

        GpuBufferDesc indexBufferDesc = vertexBufferDesc;
        indexBufferDesc.m_Size = sizeof(uint32_t) * mesh.m_Indices32.size();
        indexBufferDesc.m_Stride = sizeof(uint32_t);
        m_IndexBuffer.Create(L"IndexBuffer", indexBufferDesc, mesh.m_Indices32.data());

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
        m_GlobalRootSig[VertexData].InitAsBufferSRV(1);  // 顶点数据
        m_GlobalRootSig[IndexData].InitAsBufferSRV(2);  // 索引数据
        m_GlobalRootSig[SceneConstantBuffer].InitAsConstantBuffer(0);
        m_GlobalRootSig.Finalize(L"RayTracingGlobalRootSignature");

        CreateStateObject();

        CreateAccelerationStructure();

        CreateShaderTable();

        m_Initialized = true;
    }

    void Renderer::Shutdown()
    {
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
        raytracingShaderDesc.m_Mode = ShaderMode::SM_6_3;
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
        localRootSigAssociation.pExports = &s_HitGroupName;
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

    void Renderer::CreateAccelerationStructure()
    {
        // 给底层加速结构的几何描述
        D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC trianglesDesc{};
        trianglesDesc.Transform3x4 = 0;
        trianglesDesc.IndexFormat = DXGI_FORMAT_R32_UINT;
        trianglesDesc.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        trianglesDesc.IndexCount = m_IndexBuffer.GetCount();
        trianglesDesc.VertexCount = m_VertexBuffer.GetCount();
        trianglesDesc.IndexBuffer = m_IndexBuffer.GetGpuVirtualAddress();
        trianglesDesc.VertexBuffer.StartAddress = m_VertexBuffer.GetGpuVirtualAddress();
        trianglesDesc.VertexBuffer.StrideInBytes = m_VertexBuffer.GetStride();
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

        // 顶层加速结构的输入
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelASInputs = bottomLevelASInputs;
        topLevelASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelASInfo{};
        g_RenderContext.GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelASInputs, &topLevelASInfo);
        ASSERT(topLevelASInfo.ResultDataMaxSizeInBytes > 0);

        // 为加速结构分配显存
        GpuBufferDesc bottomLevelASDesc{};
        bottomLevelASDesc.m_Size = bottomLevelASInfo.ResultDataMaxSizeInBytes;
        bottomLevelASDesc.m_Stride = bottomLevelASDesc.m_Size;
        bottomLevelASDesc.m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
        bottomLevelASDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        m_BottomLevelAS.Create(L"BottomLevelAS", bottomLevelASDesc);
        GpuBufferDesc topLevelASDesc = bottomLevelASDesc;
        topLevelASDesc.m_Size = topLevelASInfo.ResultDataMaxSizeInBytes;
        topLevelASDesc.m_Stride = topLevelASDesc.m_Size;
        m_TopLevelAS.Create(L"TopLevelAS", topLevelASDesc);

        // 顶层加速结构的输入，使用底层加速结构作为输入
        D3D12_RAYTRACING_INSTANCE_DESC instanceDesc{};
        instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1.0f;
        instanceDesc.InstanceMask = 1;
        instanceDesc.AccelerationStructure = m_BottomLevelAS.GetGpuVirtualAddress();
        GpuResourceLocation instanceBuffer = g_RenderContext.GetCpuBufferAllocator().Allocate(sizeof(instanceDesc), D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT);
        memcpy(instanceBuffer.m_MappedAddress, &instanceDesc, sizeof(instanceDesc));
        topLevelASInputs.InstanceDescs = instanceBuffer.m_GpuAddress;

        // 分配加速结构生成需要的暂存空间
        uint64_t scratchBufferSize = (std::max)(bottomLevelASInfo.ScratchDataSizeInBytes, topLevelASInfo.ScratchDataSizeInBytes);
        GpuResourceLocation scratchBuffer = g_RenderContext.GetGpuBufferAllocator().Allocate(scratchBufferSize);

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildBottomLevelASDesc{};
        buildBottomLevelASDesc.Inputs = bottomLevelASInputs;
        buildBottomLevelASDesc.ScratchAccelerationStructureData = scratchBuffer.m_GpuAddress;
        buildBottomLevelASDesc.DestAccelerationStructureData = m_BottomLevelAS.GetGpuVirtualAddress();

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildTopLevelASDesc{};
        buildTopLevelASDesc.Inputs = topLevelASInputs;
        buildTopLevelASDesc.ScratchAccelerationStructureData = scratchBuffer.m_GpuAddress;
        buildTopLevelASDesc.DestAccelerationStructureData = m_TopLevelAS.GetGpuVirtualAddress();

        // 构建加速结构
        GraphicsCommandList cmdList{L"BuildAccelerationStructure"};
        cmdList.GetDXRCommandList()->BuildRaytracingAccelerationStructure(&buildBottomLevelASDesc, 0, nullptr);
        // 等待底层加速结构构建完毕
        cmdList.InsertUAVBarrier(m_BottomLevelAS, true);
        cmdList.GetDXRCommandList()->BuildRaytracingAccelerationStructure(&buildTopLevelASDesc, 0, nullptr);
        cmdList.ExecuteCommandList(true);
    }

    void Renderer::CreateShaderTable()
    {
        // 创建着色器表
        // 获取 Shader 的标识符
        Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> stateObjectProps{};
        ASSERT_SUCCEEDED(m_RayTracingStateObject.As(&stateObjectProps));
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
        auto size = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeof(CubeConstantBuffer);
        hitShaderTableDesc.m_Size = Math::AlignUp(size, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
        hitShaderTableDesc.m_Stride = hitShaderTableDesc.m_Size;
        std::vector<uint8_t> hitShaderTableData(hitShaderTableDesc.m_Size);
        memcpy(hitShaderTableData.data(), hitGroupIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        m_HitShaderTable.Create(L"HitShaderTable", hitShaderTableDesc, hitShaderTableData.data());
    }

    Renderer::Renderer()
        :m_TextureHeap(L"Renderer::TextureHeap", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096),
        m_LocalRootSig(1, 0),
        m_GlobalRootSig(Count, 0) {}




    
    void RayTracer::TraceRays(ComputeCommandList &cmdList)
    {
        ASSERT(m_Camera != nullptr);

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
        sceneCB.lightDir = Math::Vector4{ImguiManager::GetInstance().lightDir.Normalized(), 0};
        sceneCB.lightColor = Math::Vector4{ImguiManager::GetInstance().lightColor, 0};

        CubeConstantBuffer cubeCB{};
        cubeCB.albedo = Math::Vector4{ImguiManager::GetInstance().cubeAlbedo};
        g_Renderer.m_HitShaderTable.Update(&cubeCB, sizeof(CubeConstantBuffer), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

        cmdList.SetDescriptorTable(Renderer::RayTracingOutput, g_Renderer.m_OutputUAV);
        cmdList.SetShaderResource(Renderer::AccelerationStructure, g_Renderer.m_TopLevelAS);
        cmdList.SetShaderResource(Renderer::VertexData, g_Renderer.m_VertexBuffer);
        cmdList.SetShaderResource(Renderer::IndexData, g_Renderer.m_IndexBuffer);
        cmdList.SetDynamicConstantBuffer(Renderer::SceneConstantBuffer, sizeof(SceneConstantBuffer), &sceneCB);

        D3D12_DISPATCH_RAYS_DESC dispatchDesc{};
        dispatchDesc.HitGroupTable.StartAddress = g_Renderer.m_HitShaderTable->GetGPUVirtualAddress();
        dispatchDesc.HitGroupTable.SizeInBytes = g_Renderer.m_HitShaderTable.GetSize();
        dispatchDesc.HitGroupTable.StrideInBytes = dispatchDesc.HitGroupTable.SizeInBytes;
        dispatchDesc.MissShaderTable.StartAddress = g_Renderer.m_MissShaderTable->GetGPUVirtualAddress();
        dispatchDesc.MissShaderTable.SizeInBytes = g_Renderer.m_MissShaderTable.GetSize();
        dispatchDesc.MissShaderTable.StrideInBytes = dispatchDesc.MissShaderTable.SizeInBytes;
        dispatchDesc.RayGenerationShaderRecord.StartAddress = g_Renderer.m_RayGenShaderTable->GetGPUVirtualAddress();
        dispatchDesc.RayGenerationShaderRecord.SizeInBytes = g_Renderer.m_RayGenShaderTable.GetSize();
        dispatchDesc.Width = width;
        dispatchDesc.Height = height;
        dispatchDesc.Depth = 1;
        cmdList.GetDXRCommandList()->SetPipelineState1(g_Renderer.m_RayTracingStateObject.Get());
        cmdList.GetDXRCommandList()->DispatchRays(&dispatchDesc);
    }


}