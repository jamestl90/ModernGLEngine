#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <stack>
#include <vector>
#include <stdexcept>
#include <string>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>

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
            std::scoped_lock lock(m_mutex);
            auto it = m_resources.find(name);
            if (it != m_resources.end())
            {
                return it->second.get();
            }
            return nullptr;
        }

        T* Add(const std::string& name, T* existingResource) 
        {
            std::scoped_lock lock(m_mutex);
            if (m_resources.find(name) != m_resources.end()) {
                return existingResource; // Resource already exists
            }

            // Assign a new handle
            existingResource->SetHandle(GenerateHandle());

            auto uniqueResource = std::unique_ptr<T>(existingResource);
            m_resources[name] = std::move(uniqueResource);
            return existingResource;
        }

        T* Add(const std::string& name, const std::function<std::unique_ptr<T>()>& loader)
        {
            std::scoped_lock lock(m_mutex);
            if (m_resources.find(name) != m_resources.end())
            {
                return m_resources[name].get();
            }

            auto resource = loader();
            if (!resource)
            {
                throw std::runtime_error("Failed to load resource: " + name);
            }

            // Assign a new handle
            resource->SetHandle(GenerateHandle());

            T* rawPtr = resource.get();
            m_resources[name] = std::move(resource);
            return rawPtr;
        }

        void Remove(const std::string& name)
        {
            std::scoped_lock lock(m_mutex);
            auto it = m_resources.find(name);
            if (it != m_resources.end())
            {
                m_freeHandles.push(it->second->GetHandle());
                m_resources.erase(it);
            }
        }

        void Clear()
        {
            std::scoped_lock lock(m_mutex);
            for (auto& [name, resource] : m_resources)
            {
                m_freeHandles.push(resource->GetHandle());
            }
            m_resources.clear();
        }

    protected:
        uint32 GenerateHandle()
        {
            std::scoped_lock lock(m_mutex);
            if (!m_freeHandles.empty())
            {
                uint32_t handle = m_freeHandles.top();
                m_freeHandles.pop();
                return handle;
            }
            return m_nextHandle++;
        }

        const std::unordered_map<std::string, std::unique_ptr<T>>& GetResources() const
        {
            std::scoped_lock lock(m_mutex);
            return m_resources;
        }

    private:
        std::unordered_map<std::string, std::unique_ptr<T>> m_resources;
        std::stack<uint32> m_freeHandles;
        uint32 m_nextHandle = 0;
        mutable std::mutex m_mutex;
    };
}

#endif