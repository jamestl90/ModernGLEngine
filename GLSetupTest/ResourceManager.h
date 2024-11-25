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
        std::shared_ptr<T> Get(const std::string& name)
        {
            auto it = m_resources.find(name);
            if (it != m_resources.end())
            {
                return it->second;
            }

            return nullptr;
        }

        std::shared_ptr<T> Add(const std::string& name, const std::function<std::shared_ptr<T>()>& loader)
        {
            if (m_resources.find(name) != m_resources.end())
            {
                return m_resources[name]; // Return existing resource
            }

            auto resource = loader();
            m_resources[name] = resource;
            return resource;
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
            for (auto& [name, resource] : m_resources)
            {
                m_freeHandles.push(resource->GetHandle());
            }
            m_resources.clear();
        }

    protected:
        uint32_t GenerateHandle()
        {
            if (!m_freeHandles.empty())
            {
                uint32_t handle = m_freeHandles.top();
                m_freeHandles.pop();
                return handle;
            }
            return m_nextHandle++;
        }

    private:
        std::unordered_map<std::string, std::shared_ptr<T>> m_resources;
        std::stack<uint32_t> m_freeHandles;
        uint32_t m_nextHandle = 0;
    };
}

#endif