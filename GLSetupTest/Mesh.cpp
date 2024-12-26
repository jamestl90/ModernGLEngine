#include "Mesh.h"
#include "GraphicsAPI.h"
#include "Batch.h"

namespace JLEngine
{
	Mesh::Mesh(const string& name)
		: Resource(name)
	{
	}

	Mesh::~Mesh()
	{
	}
}