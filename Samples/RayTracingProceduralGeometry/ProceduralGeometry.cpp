#include "ProceduralGeometry.h"
#include "Graphics/RenderContext.h"
#include "Graphics/CommandList/GraphicsCommandList.h"
#include "Graphics/GraphicsCommon.h"
#include "Renderer.h"

namespace DSM{
    ProceduralGeometryManager::ProceduralGeometryManager()
    {
        GraphicsCommandList cmdList{L"ProceduralGeometry CommandList"};

        auto createBottomLevelAS = [&](
            const std::wstring& name, 
            std::span<D3D12_RAYTRACING_AABB> aabb,
            auto primitiveType) {
                // 储存所有程序几何的数据
            GpuBuffer aabbBuffer{};
            aabbBuffer.Create(name + L"_AABBBuffer", {
                .m_Size = sizeof(D3D12_RAYTRACING_AABB) * aabb.size(),
                .m_Stride = sizeof(D3D12_RAYTRACING_AABB)
                }, aabb.data());

            D3D12_RAYTRACING_GEOMETRY_AABBS_DESC aabbDesc{};
            aabbDesc.AABBCount = static_cast<UINT>(aabb.size());
            aabbDesc.AABBs.StartAddress = aabbBuffer.GetGpuVirtualAddress();
            aabbDesc.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);
            D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc{};
            geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
            geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            geometryDesc.AABBs = std::move(aabbDesc);

            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs{};
            bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
            bottomLevelInputs.NumDescs = 1;
            bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            bottomLevelInputs.pGeometryDescs = &geometryDesc;

            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelInfo{};
            g_RenderContext.GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelInfo);
            ASSERT(bottomLevelInfo.ResultDataMaxSizeInBytes > 0);

            auto bufferSize = (std::max)(bottomLevelInfo.ScratchDataSizeInBytes, bottomLevelInfo.UpdateScratchDataSizeInBytes);
            auto& bottomLevelAS = m_AnalyticPrimitives[primitiveType];
            bottomLevelAS.resultDataMaxSizeInBytes = bottomLevelInfo.ResultDataMaxSizeInBytes;
            bottomLevelAS.scratch.Create(name + L"_BottomLevelScratchBuffer", GpuBufferDesc{
                .m_Size = bufferSize,
                .m_Stride = static_cast<UINT>(bufferSize),
                .m_Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
            });
            GpuBufferDesc bufferDesc{};
            bufferDesc.m_Size = bottomLevelInfo.ResultDataMaxSizeInBytes;
            bufferDesc.m_Stride = static_cast<UINT>(bottomLevelInfo.ResultDataMaxSizeInBytes);
            bufferDesc.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            bufferDesc.m_ResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
            bottomLevelAS.accelerationStructure.Create(name + L"_BottomLevelAccelerationStructure", bufferDesc);

            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc{};
            bottomLevelBuildDesc.Inputs = bottomLevelInputs;
            bottomLevelBuildDesc.ScratchAccelerationStructureData = bottomLevelAS.scratch.GetGpuVirtualAddress();
            bottomLevelBuildDesc.DestAccelerationStructureData = bottomLevelAS.accelerationStructure.GetGpuVirtualAddress();

            cmdList.GetDXRCommandList()->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
            cmdList.ExecuteCommandList(true);
        };

        // 半径为 1 的球
        D3D12_RAYTRACING_AABB sphere{
            .MinX = -1.0f, .MinY = -1.0f, .MinZ = -1.0f,
            .MaxX =  1.0f, .MaxY =  1.0f, .MaxZ =  1.0f
        };
        createBottomLevelAS(L"Sphere", { &sphere, 1 }, RayTracing::AnalyticPrimitive::Sphere);

        D3D12_RAYTRACING_AABB quad{
            .MinX = -1.0f, .MinY = -1.0f, .MinZ = -0.0001f,
            .MaxX =  1.0f, .MaxY =  1.0f, .MaxZ =  0.0001f
        };
        createBottomLevelAS(L"Quad", { &quad, 1 }, RayTracing::AnalyticPrimitive::Quad);

        D3D12_RAYTRACING_AABB cube{
            .MinX = -1.0f, .MinY = -1.0f, .MinZ = -1.0f,
            .MaxX =  1.0f, .MaxY =  1.0f, .MaxZ =  1.0f
        };
        createBottomLevelAS(L"Cube", { &cube, 1 }, RayTracing::AnalyticPrimitive::Cube);
    }

    void ProceduralGeometryManager::AddGeometry(const ProceduralGeometryDesc &desc)
    {
        auto createInstance = [&](auto primitiveType){
            assert(desc.material != nullptr);

            const auto& sphereBottomLevelAS = m_AnalyticPrimitives[primitiveType].accelerationStructure;
            D3D12_RAYTRACING_INSTANCE_DESC instanceDesc{};
            instanceDesc.InstanceID = static_cast<UINT>(m_ProceduralGeometrys.size());
            instanceDesc.InstanceMask = RayTracing::TraceRayParameters::InstanceMark;
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE;
            instanceDesc.InstanceContributionToHitGroupIndex = m_InstanceContributionToHitGroupIndex;
            instanceDesc.AccelerationStructure = sphereBottomLevelAS.GetGpuVirtualAddress();
            DirectX::XMStoreFloat3x4(&reinterpret_cast<DirectX::XMFLOAT3X4&>(instanceDesc.Transform), desc.transform.GetLocalToWorld());
            if(desc.type == RayTracing::AnalyticPrimitive::Quad){
                instanceDesc.Transform[2][2] = 1;
            }

            D3D12_CPU_DESCRIPTOR_HANDLE defaultTexture[kNumTextures] = {
                Graphics::GetDefaultTexture(Graphics::kWhiteOpaque2D),
                Graphics::GetDefaultTexture(Graphics::kWhiteOpaque2D),
                Graphics::GetDefaultTexture(Graphics::kWhiteOpaque2D),
                Graphics::GetDefaultTexture(Graphics::kWhiteOpaque2D),
                Graphics::GetDefaultTexture(Graphics::kBlackTransparent2D),
                Graphics::GetDefaultTexture(Graphics::kDefaultNormalTex)
            };
            for(int i = 0; i < desc.textures.size(); ++i){
                if(desc.textures[i].IsValid()){
                    defaultTexture[i] = desc.textures[i].GetSRV();
                }
            }
            DescriptorHandle texHandle = g_Renderer.m_TextureHeap.Allocate(kNumTextures);

            std::uint32_t destCount = kNumTextures;
            std::uint32_t srcCount[kNumTextures] = {1,1,1,1,1,1};
            g_RenderContext.GetDevice()->CopyDescriptors(
                1, &texHandle, &destCount, destCount, defaultTexture, srcCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            GpuBufferDesc materialBufferDesc{};
            materialBufferDesc.m_Size = sizeof(MaterialConstantBuffer);
            materialBufferDesc.m_Stride = sizeof(MaterialConstantBuffer);
            ProceduralGeometry geometry{};
            geometry.type = desc.type;
            geometry.material = desc.material;
            geometry.instanceDesc = std::move(instanceDesc);
            geometry.srvOffset = g_Renderer.m_TextureHeap.GetOffsetOfHandle(texHandle);
            m_ProceduralGeometrys.push_back(std::move(geometry));
            m_InstanceContributionToHitGroupIndex += RayTracing::RayType::Count;
        };

        switch (desc.type) {
        case RayTracing::AnalyticPrimitive::Sphere:
            createInstance(RayTracing::AnalyticPrimitive::Sphere); break;
        case RayTracing::AnalyticPrimitive::Quad:
            createInstance(RayTracing::AnalyticPrimitive::Quad); break;
        case RayTracing::AnalyticPrimitive::Cube:
            createInstance(RayTracing::AnalyticPrimitive::Cube); break;
        default:
            break;
        }
    }
}