#ifndef GRAPHICS_RESOURCE_MANAGER_H
#define GRAPHICS_RESOURCE_MANAGER_H

#include "ResourceManager.h"
#include "Graphics.h"

namespace JLEngine
{
	class Graphics;

	template <class T>
	class Manager : public ResourceManager<T>
	{
	public:
		 Manager(Graphics* graphics);

		 virtual ~Manager() {}

		 void ReloadResources();

		 void UnloadResources();

	protected:

		Graphics* m_graphics;
	};
}

#include "Manager.cpp"

#endif