#ifndef GRAPHICS_RESOURCE_MANAGER_CPP
#define GRAPHICS_RESOURCE_MANAGER_CPP

#include "Manager.h"

namespace JLEngine
{
	template <class T>
	Manager<T>::Manager(Graphics* graphics)
		: m_graphics(graphics)
	{
	}


	template <class T>
	void Manager<T>::ReloadResources()
	{
		std::for_each(m_resourceList.begin(), m_resourceList.end(), [this](T* )
		{
			->Init(m_graphics);
		});
	}

	template <class T>
	void Manager<T>::UnloadResources()
	{
		std::for_each(m_resourceList.begin(), m_resourceList.end(), [this](T* )
		{
			->UnloadFromGraphics();
		});
	}
}

#endif