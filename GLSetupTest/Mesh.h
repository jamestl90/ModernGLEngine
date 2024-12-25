#ifndef MESH_H
#define MESH_H

#include "Types.h"
#include "CollisionShapes.h"
#include "VertexBuffers.h"
#include "Resource.h"

#include <memory>
#include <vector>
#include <string>

namespace JLEngine
{
	class GraphicsAPI;
	class Batch;

	struct IndirectDrawData 
	{
		uint32_t count;         
		uint32_t instanceCount; 
		uint32_t firstIndex;    
		uint32_t baseVertex;    
		uint32_t baseInstance;  
	};

	class Mesh : public Resource
	{
	public:
		Mesh(const string& name);
		Mesh(uint32_t handle, const string& name);
		~Mesh();

		void UploadToGPU(GraphicsAPI* graphics, bool freeData = false);
		void UnloadFromGPU();

        void AddBatch(std::shared_ptr<Batch> batch) { m_batches.push_back(batch); }
		const std::vector<std::shared_ptr<Batch>>& GetBatches() const { return m_batches; }

	private:

		GraphicsAPI* m_graphics;
        std::vector<std::shared_ptr<Batch>> m_batches;
	};
}

#endif