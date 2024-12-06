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
	class Graphics;
	class Batch;

	class Mesh : public Resource
	{
	public:
		Mesh(const string& name);
		Mesh(uint32 handle, const string& name);
		~Mesh();

		void UploadToGPU(Graphics* graphics, bool freeData = false);
		void UnloadFromGPU();

        void AddBatch(std::shared_ptr<Batch> batch) { m_batches.push_back(batch); }
	private:

		Graphics* m_graphics;
        std::vector<std::shared_ptr<Batch>> m_batches;
	};
}

#endif