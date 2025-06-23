#include "Model.h"
#include "Renderer.h"
#include "ConstantData.h"

using namespace DirectX;

namespace DSM {
    void Model::Render(MeshSorter& meshSorter, GpuBuffer& meshConstant, const Transform& meshTransforms)
    {
        BoundingBox modelBoudingVS{};
        Math::Matrix4 MV = meshTransforms.GetLocalToWorld() * meshSorter.GetViewMatrix();
        m_BoundingBox.Transform(modelBoudingVS, MV);
        if (!meshSorter.GetViewFrustum().Intersects(modelBoudingVS)) return;
        
        for (std::size_t i = 0; i < m_Meshes.size(); ++i) {
            const auto& mesh = m_Meshes[i];

            BoundingBox boxVS{};
            mesh->m_BoundingBox.Transform(boxVS, MV);
            
            //if (meshSorter.GetViewFrustum().Intersects(boxVS)) {
                for (const auto& [name, submesh] : mesh->m_SubMeshes) {
                    float distance = boxVS.Center.z - boxVS.Extents.z;
                    meshSorter.AddMesh(*mesh, distance,
                        meshConstant.GetGpuVirtualAddress(),
                        m_MaterialData.GetGpuVirtualAddress() + 
                            submesh.m_MaterialIndex * sizeof(MaterialConstants));
                }
            //}
        }
    }
}

