#ifndef GRAPHICS_RESOURCE_H
#define GRAPHICS_RESOURCE_H

#include "Resource.h"

namespace JLEngine
{
	class Graphics;

	class GraphicsResource : public Resource
	{
	public:
		GraphicsResource(uint32 handle, std::string& name, std::string& path)
		: Resource(handle, name, path) {}

		GraphicsResource(uint32 handle, std::string& filename)
		: Resource(handle, filename) {}

		GraphicsResource(uint32 handle)
			: Resource(handle) {}

		virtual ~GraphicsResource() {}

		virtual void Init(Graphics* graphics) = 0;

		virtual void UnloadFromGraphics() = 0;
	};
}

#endif