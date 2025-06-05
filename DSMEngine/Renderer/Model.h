#pragma once
#ifndef __MODEL_H__
#define __MODEL_H__

#include "Mesh.h"
#include "TextureManager.h"
#include "Math/Transform.h"

namespace DSM {
    class MeshSorter;

    // 模型的数据
    struct Model
    {
        void Render(MeshSorter& meshSorter, GpuBuffer& meshConstant, const Transform& meshTransforms);
        
        std::string m_Name{};
        DirectX::BoundingBox m_BoundingBox{};
        std::vector<std::shared_ptr<Mesh>> m_Meshes{};
        std::vector<std::shared_ptr<Material>> m_Materials{};
        std::vector<TextureRef> m_Textures{};
        GpuBuffer m_MaterialData{};
    };

}

#endif