#ifndef MESH_MANAGER_H
#define MESH_MANAGER_H

#include "ResourceManager.h"
#include "VertexBuffers.h"
#include "Graphics.h"
#include "Mesh.h"

namespace JLEngine
{
	class MeshManager : public ResourceManager<Mesh>
	{
    public:
        MeshManager(Graphics* graphics) : m_graphics(graphics) {}

        Mesh* LoadMeshFromData(const std::string& name, VertexBuffer& vbo, IndexBuffer& ibo);

        Mesh* CreateEmptyMesh(const std::string& name);

    private:
        Graphics* m_graphics;
	};
}

#endif