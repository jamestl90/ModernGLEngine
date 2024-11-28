#include "MeshManager.h"
#include "Mesh.h"

namespace JLEngine
{
    Mesh* JLEngine::MeshManager::LoadMeshFromData(const std::string& name, VertexBuffer& vbo, IndexBuffer& ibo)
    {
        return Add(name, [&]()
            {
                auto mesh = std::make_unique<Mesh>(GenerateHandle(), name);
                mesh->SetVertexBuffer(vbo);
                mesh->AddIndexBuffer(ibo);
                mesh->UploadToGPU(m_graphics, true);
                return mesh;
            });
    }
    Mesh* MeshManager::CreateEmptyMesh(const std::string& name)
    {
        return Add(name, [&]()
            {
                auto mesh = std::make_unique<Mesh>(GenerateHandle(), name);
                return mesh;
            });
    }
}