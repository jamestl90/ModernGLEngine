#include "Batch.h"

namespace JLEngine
{
	void Batch::SetInstanceBuffer(std::shared_ptr<InstanceBuffer> ib)
	{
		instanceBuffer = std::move(ib);
		isInstanced = true;
	}
}