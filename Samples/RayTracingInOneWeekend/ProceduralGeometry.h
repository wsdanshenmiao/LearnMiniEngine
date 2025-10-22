#pragma once
#ifndef __PROCEDURAL_GEOMETRY_H__
#define __PROCEDURAL_GEOMETRY_H__

#include "Math/Transform.h"
#include "Material.h"
#include "Renderer/TextureManager.h"
#include "RayTracingHelper.h"

namespace DSM
{
    struct ProceduralGeometryDesc
    {
        RayTracing::AnalyticPrimitive::PrimitiveType type;
        Transform transform;
        std::shared_ptr<RTMaterial> material;
        std::array<TextureRef, kNumTextures> textures;
    };

    struct ProceduralGeometry
    {
        RayTracing::AnalyticPrimitive::PrimitiveType type;
        std::shared_ptr<RTMaterial> material;
        D3D12_RAYTRACING_INSTANCE_DESC instanceDesc{};
        uint32_t srvOffset = 0;
    };

    // 统一管理所有的程序图元
    class ProceduralGeometryManager
    {
    public:
        ProceduralGeometryManager();

        void AddGeometry(const ProceduralGeometryDesc& desc);

        const std::vector<ProceduralGeometry>& GetAllGeometry() const { return m_ProceduralGeometrys; }


    private:
        // 底层加速结构
        std::array<AccelerationStructureBuffers, RayTracing::AnalyticPrimitive::Count> m_AnalyticPrimitives{};

        // 程序图元实例数据
        std::vector<ProceduralGeometry> m_ProceduralGeometrys{};
        uint32_t m_InstanceContributionToHitGroupIndex = 0;
    };
} // namespace DSM


#endif // !__PROCEDURAL_GEOMETRY_H__