#pragma once
#ifndef __MODEL_H__
#define __MODEL_H__

#include "Mesh.h"
#include "Renderer/TextureManager.h"
#include "Math/Transform.h"

namespace DSM {
    class MeshRenderer;

    // 模型的数据
    struct Model
    {
        void Render(MeshRenderer& meshRenderer, GpuBuffer& meshConstant, const Transform& meshTransforms);
        
        std::string m_Name{};
        DirectX::BoundingBox m_BoundingBox{};
        std::vector<std::shared_ptr<Mesh>> m_Meshes{};
        std::vector<std::shared_ptr<Material>> m_Materials{};
        std::vector<TextureRef> m_Textures{};
        GpuBuffer m_MaterialData{};
    };

}

#endif