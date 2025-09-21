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
        m_LocalRootSig[LocalRootSignature::Triangle::Material].InitAsConstants(1, sizeof(RayTracing::Material) / 4);
        m_LocalRootSig[LocalRootSignature::Triangle::IndexBuffer].InitAsBufferSRV(1);
        m_LocalRootSig[LocalRootSignature::Triangle::NormalBuffer].InitAsBufferSRV(2);
        m_LocalRootSig[LocalRootSignature::Triangle::UVBuffer].InitAsBufferSRV(3);
        m_LocalRootSig[LocalRootSignature::Triangle::Textures].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, kNumTextures);
        m_LocalRootSig.Finalize(L"RayTracingLocalRootSignature", D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
        m_GlobalRootSig[GlobalRootSignature::RayTracingOutput].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);  // RayTracingOutput
        m_GlobalRootSig[GlobalRootSignature::AccelerationStructure].InitAsBufferSRV(0);  // 加速结构
        m_GlobalRootSig[GlobalRootSignature::SceneConstantBuffer].InitAsConstantBuffer(0);
        m_GlobalRootSig.Finalize(L"RayTracingGlobalRootSignature");

        CreateStateObject();

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
        // 1 - Hit group
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

        std::vector<D3D12_EXPORT_DESC> exportDescs{};
        D3D12_EXPORT_DESC exportDesc{};
        exportDesc.Name = s_RayGenShaderName;
        exportDescs.push_back(std::move(exportDesc));
        for(int i = 0; i < s_MissShaderName.size(); ++i){
            D3D12_EXPORT_DESC desc{};
            desc.Name = s_MissShaderName[i];
            exportDescs.push_back(std::move(desc));
        }
        for(int i = 0; i < s_ClosestHitShaderName_Triangle.size(); ++i){
            D3D12_EXPORT_DESC desc{};
            desc.Name = s_ClosestHitShaderName_Triangle[i];
            exportDescs.push_back(std::move(desc));
        }

        // DXIL library
        D3D12_DXIL_LIBRARY_DESC dxilLibDesc{};
        dxilLibDesc.DXILLibrary = shaderLib;
        dxilLibDesc.NumExports = exportDescs.size();
        dxilLibDesc.pExports = exportDescs.data();
        // 使用默认导出
        D3D12_STATE_SUBOBJECT libSubobject{};
        libSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        libSubobject.pDesc = &dxilLibDesc;
        subobjects.push_back(std::move(libSubobject));


        std::vector<D3D12_HIT_GROUP_DESC> hitGroupDescs{};
        // Hit group
        for(int i = 0; i < RayTracing::RayType::Count; i++){
            auto& hitGroupDesc = hitGroupDescs.emplace_back();
            hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
            hitGroupDesc.HitGroupExport = s_HitGroupName_Triangle[i];
            hitGroupDesc.ClosestHitShaderImport = s_ClosestHitShaderName_Triangle[GeometryType::Triangle];

            D3D12_STATE_SUBOBJECT hitGroupSubobject{};
            hitGroupSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            hitGroupSubobject.pDesc = &hitGroupDesc;
            subobjects.push_back(std::move(hitGroupSubobject));
        }
    
    
        // Shader config
        D3D12_RAYTRACING_SHADER_CONFIG shaderConfig{};
        shaderConfig.MaxAttributeSizeInBytes = sizeof(RayTracing::ProceduralPrimitiveAttributes); // 三角形的重心坐标
        shaderConfig.MaxPayloadSizeInBytes = sizeof(RayTracing::RayPayload); // 光线的颜色

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
        localRootSigAssociation.NumExports = s_HitGroupName_Triangle.size();
        localRootSigAssociation.pExports = s_HitGroupName_Triangle.data();
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
        pipelineConfig.MaxTraceRecursionDepth = s_MaxRecursionDepth;  // 最大递归深度

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
        m_LocalRootSig(LocalRootSignature::Triangle::Count, 0),
        m_GlobalRootSig(GlobalRootSignature::Count, 0) {}




    
    void RayTracer::TraceRays(ComputeCommandList &cmdList)
    {
        ASSERT(m_Camera != nullptr);
        if(m_Models.empty()) 
            return;

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
        sceneCB.lightDir = Math::Vector4{ImguiManager::GetInstance().lightDir.Normalized(), 0};
        sceneCB.lightColor = Math::Vector4{ImguiManager::GetInstance().lightColor, 0};

        cmdList.SetDescriptorTable(GlobalRootSignature::RayTracingOutput, g_Renderer.m_OutputUAV);
        cmdList.SetShaderResource(GlobalRootSignature::AccelerationStructure, m_TopLevelAS.accelerationStructure);
        cmdList.SetDynamicConstantBuffer(GlobalRootSignature::SceneConstantBuffer, sizeof(sceneCB), &sceneCB);

        D3D12_DISPATCH_RAYS_DESC dispatchDesc{};
        dispatchDesc.HitGroupTable.StartAddress = m_HitGroupShaderTable->GetGPUVirtualAddress();
        dispatchDesc.HitGroupTable.SizeInBytes = m_HitGroupShaderTable.GetSize();
        dispatchDesc.HitGroupTable.StrideInBytes = m_HitGroupShaderTable.GetStride();
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
        m_Models.push_back(std::move(model));
        CreateAccelerationStructure();
        CreateShaderTable();
    }

    void RayTracer::CreateAccelerationStructure()
    {
        bool firstCreate = m_TopLevelAS.accelerationStructure.GetResource() == nullptr;

        m_BottomLevelASs.clear();
        GraphicsCommandList cmdList(L"CreateAccelerationStructure");

        std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
        uint32_t instanceContributionToHitGroupIndex = 0;
        // 创建 BLAS
        for(const auto& model : m_Models) {
            for(const auto& mesh : model->meshes) {
                for(const auto& [name, submesh] : mesh->m_SubMeshes){
                    AccelerationStructureBuffers buffers{};

                    D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC triangleDesc{};
                    // 顶点缓冲区
                    triangleDesc.VertexBuffer.StartAddress = 
                        mesh->m_PositionStream.BufferLocation + submesh.m_VertexOffset * sizeof(DirectX::XMFLOAT3);
                    triangleDesc.VertexBuffer.StrideInBytes = mesh->m_PositionStream.StrideInBytes;
                    triangleDesc.VertexCount = submesh.m_VertexCount;
                    triangleDesc.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
                    triangleDesc.IndexBuffer = mesh->m_IndexBufferViews.BufferLocation + submesh.m_IndexOffset * sizeof(uint32_t);
                    triangleDesc.IndexCount = submesh.m_IndexCount;
                    triangleDesc.IndexFormat = mesh->m_IndexBufferViews.Format;

                    auto& geometryDesc = geometryDescs.emplace_back();
                    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
                    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
                    geometryDesc.Triangles = std::move(triangleDesc);


                    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs{};
                    bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
                    bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
                    bottomLevelInputs.NumDescs = 1;
                    bottomLevelInputs.pGeometryDescs = &geometryDesc;
                    bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
                
                    // 获取加速结构的相关信息
                    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelASInfo{};
                    g_RenderContext.GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelASInfo);
                    ASSERT(bottomLevelASInfo.ResultDataMaxSizeInBytes > 0);

                    buffers.resultDataMaxSizeInBytes = bottomLevelASInfo.ResultDataMaxSizeInBytes;
                    GpuBufferDesc bottomLevelASBufferDesc{};
                    bottomLevelASBufferDesc.m_Size = buffers.resultDataMaxSizeInBytes;
                    bottomLevelASBufferDesc.m_Stride = bottomLevelASBufferDesc.m_Size;
                    bottomLevelASBufferDesc.m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
                    bottomLevelASBufferDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                    auto wName = Utility::UTF8ToWString(model->name + "_" + mesh->m_Name + "_" + name);
                    buffers.accelerationStructure.Create(L"BottomLevelAS" + wName, bottomLevelASBufferDesc);

                    GpuBufferDesc scratchBufferDesc = bottomLevelASBufferDesc;
                    scratchBufferDesc.m_Size = (std::max)(bottomLevelASInfo.ScratchDataSizeInBytes, bottomLevelASInfo.UpdateScratchDataSizeInBytes);
                    scratchBufferDesc.m_Stride = scratchBufferDesc.m_Size;
                    buffers.scratch.Create(L"BottomLevelASScratch" + wName, scratchBufferDesc);

                    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildASDesc{};
                    bottomLevelBuildASDesc.Inputs = bottomLevelInputs;
                    bottomLevelBuildASDesc.DestAccelerationStructureData = buffers.accelerationStructure.GetGpuVirtualAddress();
                    bottomLevelBuildASDesc.ScratchAccelerationStructureData = buffers.scratch.GetGpuVirtualAddress();

                    // 创建底层加速结构
                    cmdList.GetDXRCommandList()->BuildRaytracingAccelerationStructure(&bottomLevelBuildASDesc, 0, nullptr);
                
                    // 添加该底层加速结构的实例
                    auto& instanceDesc = instanceDescs.emplace_back();
                    instanceDesc.AccelerationStructure = buffers.accelerationStructure.GetGpuVirtualAddress();
                    instanceDesc.InstanceMask = 1;
                    instanceDesc.InstanceContributionToHitGroupIndex = instanceContributionToHitGroupIndex;
                    instanceContributionToHitGroupIndex += bottomLevelInputs.NumDescs * RayTracing::RayType::Count;
                    auto dest = reinterpret_cast<DirectX::XMFLOAT3X4*>(instanceDesc.Transform);
                    DirectX::XMStoreFloat3x4(dest, model->transform.GetLocalToWorld());

                    m_BottomLevelASs.push_back(std::move(buffers));
                }
            }
        }

        GpuBufferDesc instanceBufferDesc{};
        instanceBufferDesc.m_Size = instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
        instanceBufferDesc.m_Stride = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
        m_TopLevelAS.instanceDesc.Create(L"InstanceBuffer", instanceBufferDesc, instanceDescs.data());
        
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs{};
        topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        topLevelInputs.NumDescs = instanceDescs.size();
        topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        topLevelInputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
        if (!firstCreate){
            topLevelInputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
        }
        topLevelInputs.InstanceDescs = m_TopLevelAS.instanceDesc.GetGpuVirtualAddress();

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelASInfo{};
        g_RenderContext.GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelASInfo);
        ASSERT(topLevelASInfo.ResultDataMaxSizeInBytes > 0);
        m_TopLevelAS.resultDataMaxSizeInBytes = topLevelASInfo.ResultDataMaxSizeInBytes;

        GpuBufferDesc topLevelASBufferDesc{};
        topLevelASBufferDesc.m_Size = topLevelASInfo.ResultDataMaxSizeInBytes;
        topLevelASBufferDesc.m_Stride = topLevelASBufferDesc.m_Size;
        topLevelASBufferDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        topLevelASBufferDesc.m_ResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        m_TopLevelAS.accelerationStructure.Create(L"TopLevelAS", topLevelASBufferDesc);

        GpuBufferDesc scratchBufferDesc = topLevelASBufferDesc;
        scratchBufferDesc.m_Size = (std::max)(topLevelASInfo.ScratchDataSizeInBytes, topLevelASInfo.UpdateScratchDataSizeInBytes);
        scratchBufferDesc.m_Stride = scratchBufferDesc.m_Size;
        m_TopLevelAS.scratch.Create(L"ScratchBuffer", scratchBufferDesc);

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildASDesc{};
        topLevelBuildASDesc.Inputs = topLevelInputs;
        topLevelBuildASDesc.DestAccelerationStructureData = m_TopLevelAS.accelerationStructure.GetGpuVirtualAddress();
        topLevelBuildASDesc.ScratchAccelerationStructureData = m_TopLevelAS.scratch.GetGpuVirtualAddress();
        if(!firstCreate){
            topLevelBuildASDesc.SourceAccelerationStructureData = m_TopLevelAS.accelerationStructure.GetGpuVirtualAddress();
        }

        cmdList.GetDXRCommandList()->BuildRaytracingAccelerationStructure(&topLevelBuildASDesc, 0, nullptr);
        cmdList.ExecuteCommandList(true);
    }
    
    void RayTracer::CreateShaderTable()
    {
        /*************--------- Shader table layout -------*******************
        | --------------------------------------------------------------------
        | Shader table - HitGroupShaderTable: 
        | [0] : MyHitGroup_Model0
        | [1] : MyHitGroup_Model1
        | ...
        | --------------------------------------------------------------------
        **********************************************************************/

        Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> stateObjectProps;
        ASSERT_SUCCEEDED(g_Renderer.m_RayTracingStateObject.As(&stateObjectProps));
        void* rayGenShaderID = stateObjectProps->GetShaderIdentifier(Renderer::s_RayGenShaderName);
        std::array<void*, RayTracing::RayType::Count> missShaderIDs{};
        std::array<void*, RayTracing::RayType::Count> hitGroupShaderIDs{};
        for(int i = 0; i < g_Renderer.s_MissShaderName.size(); i++) {
            missShaderIDs[i] = stateObjectProps->GetShaderIdentifier(Renderer::s_MissShaderName[i]);
        }
        for(int i = 0; i < g_Renderer.s_HitGroupName_Triangle.size(); i++) {
            hitGroupShaderIDs[i] = stateObjectProps->GetShaderIdentifier(Renderer::s_HitGroupName_Triangle[i]);
        }

        uint32_t shaderIdSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

        GpuBufferDesc rayGenShaderTableDesc{};
        rayGenShaderTableDesc.m_Size = shaderIdSize;
        rayGenShaderTableDesc.m_Stride = shaderIdSize;
        m_RayGenShaderTable.Create(L"RayGenShaderTable", rayGenShaderTableDesc, rayGenShaderID);

        GpuBufferDesc missShaderTableDesc = rayGenShaderTableDesc;
        missShaderTableDesc.m_Size = shaderIdSize * missShaderIDs.size();
        missShaderTableDesc.m_Stride = shaderIdSize;
        m_MissShaderTable.Create(L"MissShaderTable", missShaderTableDesc, missShaderIDs.data());

        // uint32_t hitGroupShaderRecordSize = shaderIdSize + LocalRootSignature::MaxRootArgumentsSize();
        // hitGroupShaderRecordSize = Math::AlignUp(hitGroupShaderRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
        // std::vector<uint8_t> hitGroupRecordData(hitGroupShaderRecordSize);
        auto hitGroupShaderRecordSize = shaderIdSize + LocalRootSignature::MaxRootArgumentsSize();
        std::vector<uint8_t> hitGroupRecordData(hitGroupShaderRecordSize);
        std::vector<uint8_t> hitGroupTableData{};
        for (const auto& model : m_Models){
            for(const auto& mesh : model->meshes){
                for(const auto& [name, submesh] : mesh->m_SubMeshes) {
                    for(int i = 0; i < RayTracing::RayType::Count; i++) {
                        // Fill hitGroupRecordData with shader IDs and root arguments
                        memcpy(hitGroupRecordData.data(), hitGroupShaderIDs[i], shaderIdSize);
                        LocalRootSignature::Triangle::RootArguments rootArgs{};
                        auto meshMat = model->materials[submesh.m_MaterialIndex];
                        rootArgs.material.baseColor = meshMat->baseColor;
                        rootArgs.material.emissiveColor = meshMat->emissiveColor;
                        rootArgs.material.metallicFactor = meshMat->metallicFactor;
                        rootArgs.material.roughnessFactor = meshMat->roughnessFactor;
                        rootArgs.material.normalTexScale = meshMat->normalTexScale;
                        rootArgs.indexBuffer = mesh->m_IndexBufferViews.BufferLocation + submesh.m_IndexOffset * sizeof(uint32_t);
                        rootArgs.normalBuffer = mesh->m_NormalStream.BufferLocation + submesh.m_VertexOffset * sizeof(DirectX::XMFLOAT3);
                        rootArgs.uvBuffer = mesh->m_UVStream.BufferLocation + submesh.m_VertexOffset * sizeof(DirectX::XMFLOAT2);
                        rootArgs.textures = g_Renderer.m_TextureHeap[submesh.m_SRVTableOffset];
                        memcpy(hitGroupRecordData.data() + shaderIdSize, &rootArgs, LocalRootSignature::MaxRootArgumentsSize());
                        hitGroupTableData.append_range(hitGroupRecordData);
                    }
                }
            }
        }
        
        GpuBufferDesc triangleHitGroupDesc = rayGenShaderTableDesc;
        triangleHitGroupDesc.m_Size = Math::AlignUp(hitGroupTableData.size(), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
        triangleHitGroupDesc.m_Stride = hitGroupShaderRecordSize;
        hitGroupRecordData.resize(triangleHitGroupDesc.m_Size);
        m_HitGroupShaderTable.Create(L"HitGroupShaderTable", triangleHitGroupDesc, hitGroupTableData.data());
    }


}