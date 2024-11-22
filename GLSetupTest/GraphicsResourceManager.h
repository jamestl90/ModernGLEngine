#ifndef GRAPHICS_RESOURCE_MANAGER_H
#define GRAPHICS_RESOURCE_MANAGER_H

#include "ResourceManager.h"
#include "Graphics.h"

namespace JLEngine
{
	class Graphics;

	template <class T>
	class GraphicsResourceManager : public ResourceManager<T>
	{
	public:
		 explicit GraphicsResourceManager(Graphics* graphics);

		 virtual ~GraphicsResourceManager() override {}

		 void ReloadResources();

		 void UnloadResources();

	protected:

		Graphics* m_graphics;
	};
}

#include "GraphicsResourceManager.cpp"

#endif