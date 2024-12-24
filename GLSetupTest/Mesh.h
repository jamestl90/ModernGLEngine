#ifndef MESH_H
#define MESH_H

#include "Types.h"
#include "CollisionShapes.h"
#include "VertexBuffers.h"
#include "InstanceBuffer.h"
#include "Resource.h"

#include <memory>
#include <vector>
#include <string>

namespace JLEngine
{
	class Graphics;
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

		void UploadToGPU(Graphics* graphics, bool freeData = false);
		void UnloadFromGPU();

        void AddBatch(std::shared_ptr<Batch> batch) { m_batches.push_back(batch); }
		const std::vector<std::shared_ptr<Batch>>& GetBatches() const { return m_batches; }

		void SetInstanced(bool instanced)
		{
			m_instanced = instanced;
		}

		void SetInstanceBuffer(std::shared_ptr<InstanceBuffer> instanceBuffer);
		bool IsInstanced() const { return m_instanced; }
	private:

		Graphics* m_graphics;
        std::vector<std::shared_ptr<Batch>> m_batches;
		bool m_instanced;
	};
}

#endif