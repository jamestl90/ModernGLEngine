#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <stack>
#include <vector>
#include <stdexcept>
#include <string>

#include "Types.h"

using std::string;
using std::vector;
using std::stack;

namespace JLEngine
{
	template <class T>
	class ResourceManager
	{
	public:
		 ResourceManager();
		 virtual ~ResourceManager();

		 T* Add(string& name, string& path);
		 T* Add(string& name, T* res);
		 void Clear();
		 void Remove(uint32 handle);

		// slow linear search
		 void Remove(const std::string& name, string path = "");

		 T* GetResource(uint32 handle);
		 T* GetResource(string& name);
		 T* GetResource(string& name, string path = "");

		 virtual void ReloadResources() = 0;

		 vector<T*>& GetResourceList() { return m_resourceList; }

		 T* operator [] (uint32 id);

	protected:

		vector<T*> m_resourceList;
		stack<uint32> m_freeHandles;
	};

#include "ResourceManager.cpp"

#endif