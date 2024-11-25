#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <stack>
#include <vector>
#include <stdexcept>
#include <string>
#include <iostream>

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
		virtual ~ResourceManager() { Clear(); }

		T* Add(string name, string path);
		T* Add(string name, T* res);
		void Clear();
		void Remove(uint32 handle);

		// slow linear search
		void Remove(const std::string name, string path);

		T* GetResource(uint32 handle);
		T* GetResource(string name);
		T* GetResource(string name, string path);

		virtual void ReloadResources() = 0;

		vector<T*>& GetResourceList() { return m_resourceList; }

		T* operator [] (uint32 id);

	protected:

		vector<T*> m_resourceList;
		stack<uint32> m_freeHandles;
	};

	template <class T>
	ResourceManager<T>::ResourceManager()
	{
	}

	template <class T>
	void ResourceManager<T>::Clear()
	{
		for (auto& resource : m_resourceList) {
			if (resource != nullptr) {
				delete resource;
				resource = nullptr;
			}
		}

		m_resourceList.clear();

		while (!m_freeHandles.empty())
			m_freeHandles.pop();
	}

	template <class T>
	T* ResourceManager<T>::GetResource(uint32 handle)
	{
		if (handle < 0 || handle > m_resourceList.size())
			return nullptr;

		T* resource = nullptr;

		try
		{
			resource = m_resourceList.at(handle);
		}
		catch (std::out_of_range& oor)
		{
			std::cout << oor.what() << std::endl;
			return nullptr;
		}
		return resource;
	}

	template <class T>
	T* ResourceManager<T>::GetResource(string name, string path)
	{
		for (auto it = m_resourceList.begin(); it != m_resourceList.end(); it++)
		{
			if ((*it)->GetName() == name && (*it)->GetFilePath() == path)
			{
				return (*it);
			}
		}
		return nullptr;
	}

	template <class T>
	T* ResourceManager<T>::GetResource(string name)
	{
		for (auto it = m_resourceList.begin(); it != m_resourceList.end(); it++)
		{
			if ((*it)->GetName() == name)
				return (*it);
		}
		return nullptr;
	}

	template <class T>
	void ResourceManager<T>::Remove(const std::string fullName, string path)
	{
		bool noPath = false;

		if (path == "")
		{
			noPath = true;
		}

		for (uint32 i = 0; i < m_resourceList.size(); i++)
		{
			if (m_resourceList.at(i)->GetName() == fullName)
			{
				if (noPath)
				{
					Remove(m_resourceList.at(i)->GetHandle());
				}
				else if (m_resourceList.at(i)->GetFilePath() == path)
				{
					Remove(m_resourceList.at(i)->GetHandle());
				}
				break;
			}
		}
	}

	template <class T>
	void ResourceManager<T>::Remove(uint32 handle)
	{
		T* resource = m_resourceList.at(handle);

		if (resource != nullptr)
		{
			resource->DecrementCount();

			if (resource->GetReferenceCount() == 0)
			{
				m_freeHandles.push(resource->GetHandle());

				delete resource;

				m_resourceList[handle] = nullptr;
			}
		}
	}

	template <class T>
	T* ResourceManager<T>::Add(string name, T* res)
	{
		size_t handle;
		bool freeHandle = false;

		if (!m_freeHandles.empty())
		{
			handle = m_freeHandles.top();
			m_freeHandles.pop();
			freeHandle = true;
		}
		else
		{
			handle = m_resourceList.size();
			freeHandle = false;
		}

		res->SetHandle((uint32)handle);

		if (!freeHandle)
			m_resourceList.push_back(res);
		else
			m_resourceList[handle] = res;

		return res;
	}

	template <class T>
	T* ResourceManager<T>::Add(string name, string path)
	{
		T* resource = GetResource(name, path);

		if (resource != nullptr)
		{
			resource->IncrementCount();
			return resource;
		}
		else
		{
			size_t handle;
			bool freeHandle = false;

			if (!m_freeHandles.empty())
			{
				handle = m_freeHandles.top();
				m_freeHandles.pop();
				freeHandle = true;
			}
			else
			{
				handle = m_resourceList.size();
				freeHandle = false;
			}

			T* newResource = nullptr;

			newResource = new T((uint32)handle, name, path);

			if (!freeHandle)
				m_resourceList.push_back(newResource);
			else
				m_resourceList[handle] = newResource;

			return newResource;
		}
		return nullptr;
	}

	template <class T>
	T* ResourceManager<T>::operator [] (uint32 id)
	{
		if (id < 0 || id > m_resourceList.size())
			return nullptr;

		T* resource = nullptr;

		try
		{
			resource = m_resourceList.at(id);
		}
		catch (std::out_of_range& outOfRange)
		{
			return nullptr;
		}
		return resource;
	}
}

#endif