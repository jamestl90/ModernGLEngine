#include "Mesh.h"
#include "Graphics.h"
#include "Batch.h"

namespace JLEngine
{
	Mesh::Mesh(const string& name)
		: Resource(name), m_graphics(nullptr)
	{
	}

	Mesh::Mesh(uint32 handle, const string& name)
		: Resource(handle, name), m_graphics(nullptr)
	{

	}

	Mesh::~Mesh()
	{
		UnloadFromGPU();
	}

	void Mesh::UploadToGPU(Graphics* graphics, bool freeData)
	{
		if (!graphics || m_batches.empty()) 
		{
			std::cerr << "Failed to upload Mesh '" << GetName() << "' to GPU: Graphics or batches are invalid.\n";
			return;
		}

		m_graphics = graphics;

		for (auto& batch : m_batches) 
		{
			if (!batch->IsValid()) {
				std::cerr << "Invalid batch found in Mesh '" << GetName() << "' during upload.\n";
				continue;
			}

			m_graphics->CreateVertexBuffer(*batch->GetVertexBuffer().get());
			m_graphics->CreateIndexBuffer(*batch->GetIndexBuffer().get());
			if (batch->HasInstanceBuffer()) {
				batch->GetInstanceBuffer()->UploadToGPU(m_graphics, {}); 
			}
		}

		if (freeData) {
			// Release CPU-side resources if requested
			m_batches.clear();
		}
	}

	void Mesh::UnloadFromGPU()
	{
		if (m_graphics)
		{
			m_graphics->DisposeMesh(this);
		}
	}
}