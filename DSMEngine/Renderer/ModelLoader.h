#pragma once
#ifndef __MODELLOADER_H__
#define __MODELLOADER_H__

#include "Model.h"

namespace DSM{
    namespace Geometry {
        struct GeometryMesh;
    }
    
    std::shared_ptr<Model> LoadModel(const std::string& filename);
    std::shared_ptr<Model> LoadModelFromeGeometry(const std::string& name, const Geometry::GeometryMesh& geometryMesh);
}


#endif