#include "Mesh.h"
#include "GraphicsAPI.h"

namespace JLEngine
{
	Mesh::Mesh(const string& name)
		: Resource(name)
	{
	}

	Mesh::~Mesh()
	{
	}

	std::string MakeKey(const std::string& meshName, const SubMesh& subMesh)
	{
		return meshName + std::to_string(subMesh.attribKey) + "_" + std::to_string(subMesh.materialHandle);
	}
}