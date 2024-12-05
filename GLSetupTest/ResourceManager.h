#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <stack>
#include <vector>
#include <stdexcept>
#include <string>
#include <iostream>
#include <unordered_map>
#include <functional>

#include "Types.h"

using std::string;
using std::vector;
using std::stack;

namespace JLEngine
{
    template <typename T>
    class ResourceManager
    {
    public:
        T* Get(const std::string& name)
        {
            auto it = m_resources.find(name);
            if (it != m_resources.end())
            {
                return it->second.get(); // Return raw pointer
            }

            return nullptr;
        }

        T* Add(const std::string& name, const std::function<std::unique_ptr<T>()>& loader)
        {
            if (m_resources.find(name) != m_resources.end())
            {
                return m_resources[name].get(); // Return existing resource as raw pointer
            }

            auto resource = loader();
            T* rawPtr = resource.get();
            m_resources[name] = std::move(resource); // Store unique_ptr
            return rawPtr; // Return raw pointer
        }

        void Remove(const std::string& name)
        {
            auto it = m_resources.find(name);
            if (it != m_resources.end())
            {
                m_freeHandles.push(it->second->GetHandle()); // Recycle handle
                m_resources.erase(it);
            }
        }

        void Clear()
        {
            for (auto it = m_resources.begin(); it != m_resources.end(); ++it)
            {
                m_freeHandles.push(it->second->GetHandle()); 
            }
            m_resources.clear();
        }

    protected:
        uint32 GenerateHandle()
        {
            if (!m_freeHandles.empty())
            {
                uint32_t handle = m_freeHandles.top();
                m_freeHandles.pop();
                return handle;
            }
            return m_nextHandle++;
        }

        const std::unordered_map<std::string, std::unique_ptr<T>>& GetResources() { return m_resources; }

    private:
        std::unordered_map<std::string, std::unique_ptr<T>> m_resources;
        std::stack<uint32> m_freeHandles;
        uint32 m_nextHandle = 0;
    };
}

#endif