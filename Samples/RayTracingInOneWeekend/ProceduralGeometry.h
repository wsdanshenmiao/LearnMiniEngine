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
        IntersectionShaderType::Enum intersectionShaderType = IntersectionShaderType::AnalyticPrimitive;
        RayTracing::AnalyticPrimitive::PrimitiveType type = RayTracing::AnalyticPrimitive::Sphere;
        Transform transform;
        std::shared_ptr<RTMaterial> material = nullptr;
        std::array<TextureRef, kNumTextures> textures;
    };

    struct ProceduralGeometry
    {
        IntersectionShaderType::Enum intersectionShaderType = IntersectionShaderType::AnalyticPrimitive;
        RayTracing::AnalyticPrimitive::PrimitiveType type = RayTracing::AnalyticPrimitive::Sphere;
        std::shared_ptr<RTMaterial> material = nullptr;
        D3D12_RAYTRACING_INSTANCE_DESC instanceDesc{};
        uint32_t srvOffset = 0;
    };

    // 统一管理所有的程序图元
    class ProceduralGeometryManager
    {
    public:
        struct ImportanceSamplingObject
        {
            RayTracing::ImportanceSampling::ImportanceSamplingPrimitiveType type = RayTracing::ImportanceSampling::Sphere;
            Math::Matrix4 objToWorld{};
        };

        ProceduralGeometryManager();

        void AddGeometry(const ProceduralGeometryDesc& desc);
        void AddImportanceSamplingObject(std::span<ImportanceSamplingObject> objs) 
        {
            if(!objs.empty()) {
                m_ImportanceSamplingObjects.insert(m_ImportanceSamplingObjects.end(), objs.begin(), objs.end());
            }
        }
        void AddImportanceSamplingObject(ImportanceSamplingObject obj) 
        {
            m_ImportanceSamplingObjects.push_back(std::move(obj));
        }

        const std::vector<ProceduralGeometry>& GetAllGeometry() const { return m_ProceduralGeometrys; }

        const std::vector<ImportanceSamplingObject>& GetAllImportanceSamplingObjects() const { return m_ImportanceSamplingObjects; }


    private:
        // 底层加速结构
        std::array<AccelerationStructureBuffers, RayTracing::AnalyticPrimitive::Count> m_AnalyticPrimitives{};

        // 程序图元实例数据
        std::vector<ProceduralGeometry> m_ProceduralGeometrys{};
        std::vector<ImportanceSamplingObject> m_ImportanceSamplingObjects{};
        uint32_t m_InstanceContributionToHitGroupIndex = 0;
    };
} // namespace DSM


#endif // !__PROCEDURAL_GEOMETRY_H__