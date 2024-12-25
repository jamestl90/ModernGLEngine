#include "Mesh.h"
#include "GraphicsAPI.h"
#include "Batch.h"

namespace JLEngine
{
	Mesh::Mesh(const string& name)
		: Resource(name), m_graphics(nullptr)
	{
	}

	Mesh::Mesh(uint32_t handle, const string& name)
		: Resource(handle, name), m_graphics(nullptr)
	{

	}

	Mesh::~Mesh()
	{
		UnloadFromGPU();
	}

	void Mesh::UploadToGPU(GraphicsAPI* graphics, bool freeData)
	{
		if (!graphics || m_batches.empty()) 
		{
			std::cerr << "Failed to upload Mesh '" << GetName() << "' to GPU: Graphics or batches are invalid.\n";
			return;
		}

		m_graphics = graphics;

		for (auto& batch : m_batches) 
		{
			if (!batch->IsValid()) 
			{
				std::cerr << "Invalid batch found in Mesh '" << GetName() << "' during upload.\n";
				continue;
			}
			m_graphics->CreateBatch(*batch);
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
			for (auto& batch : m_batches)
			{
				if (batch->IsValid())
				{
					m_graphics->DisposeBatch(*batch);
				}
			}
		}
	}
}