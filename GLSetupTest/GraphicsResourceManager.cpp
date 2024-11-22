#ifndef GRAPHICS_RESOURCE_MANAGER_CPP
#define GRAPHICS_RESOURCE_MANAGER_CPP

#include "GraphicsResourceManager.h"

template class JLEngine::GraphicsResourceManager<JLEngine::ShaderProgram>;
template class JLEngine::GraphicsResourceManager<JLEngine::Texture>;

namespace JLEngine
{
	template <class T>
	GraphicsResourceManager<T>::GraphicsResourceManager(Graphics* graphics)
		: m_graphics(graphics)
	{
	}


	template <class T>
	void GraphicsResourceManager<T>::ReloadResources()
	{
		for (T* graphicsResource : this->m_resourceList)
		{
			if (graphicsResource) 
			{
				graphicsResource->Init(m_graphics);
			}
		}
	}

	template <class T>
	void GraphicsResourceManager<T>::UnloadResources()
	{
		for (T* graphicsResource : this->m_resourceList)
		{
			if (graphicsResource) 
			{
				graphicsResource->UnloadFromGraphics();
			}
		};
	}
}

#endif