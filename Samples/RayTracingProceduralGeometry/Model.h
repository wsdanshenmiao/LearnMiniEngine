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
        std::string name{};
        DirectX::BoundingBox boundingBox{};
        std::vector<std::shared_ptr<Mesh>> meshes{};
        std::vector<std::shared_ptr<Material>> materials{};
        std::vector<TextureRef> textures{};
        GpuBuffer materialData{};

        Transform transform{};
    };

}

#endif