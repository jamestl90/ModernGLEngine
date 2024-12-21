#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <stack>
#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <stdexcept>

#include "Types.h"

namespace JLEngine
{
    template <typename T>
    class ResourceManager
    {
    public:
        T* Get(uint32_t id)
        {
            std::scoped_lock lock(m_mutex);
            auto it = m_resourcesById.find(id);
            return (it != m_resourcesById.end()) ? it->second.get() : nullptr;
        }

        T* Get(const std::string& name)
        {
            std::scoped_lock lock(m_mutex);
            auto it = m_nameToId.find(name);
            return (it != m_nameToId.end()) ? Get(it->second) : nullptr;
        }

        uint32_t Add(const std::string& name, T* existingResource)
        {
            std::scoped_lock lock(m_mutex);
            if (m_nameToId.find(name) != m_nameToId.end())
            {
                return m_nameToId[name];
            }

            uint32_t id = GenerateHandle();
            existingResource->SetHandle(id);

            m_resourcesById[id] = std::unique_ptr<T>(existingResource);
            m_nameToId[name] = id;
            m_idToName[id] = name; // Add reverse lookup

            return id;
        }

        T* Add(const std::string& name, const std::function<std::unique_ptr<T>()>& loader)
        {
            std::scoped_lock lock(m_mutex);

            auto nameToIdIt = m_nameToId.find(name);
            if (nameToIdIt != m_nameToId.end())
            {
                uint32_t id = nameToIdIt->second;
                return m_resourcesById[id].get();
            }

            auto resource = loader();
            if (!resource)
            {
                throw std::runtime_error("Failed to load resource: " + name);
            }

            uint32_t id = GenerateHandle();
            resource->SetHandle(id);

            T* rawPtr = resource.get();
            m_resourcesById[id] = std::move(resource);
            m_nameToId[name] = id;
            m_idToName[id] = name; // Add reverse lookup

            return rawPtr;
        }

        void Remove(uint32_t id)
        {
            std::scoped_lock lock(m_mutex);
            auto it = m_resourcesById.find(id);
            if (it != m_resourcesById.end())
            {
                auto nameIt = m_idToName.find(id);
                if (nameIt != m_idToName.end())
                {
                    m_nameToId.erase(nameIt->second);
                    m_idToName.erase(nameIt);
                }

                m_freeHandles.push(id);
                m_resourcesById.erase(it);
            }
        }

        void Remove(const std::string& name)
        {
            std::scoped_lock lock(m_mutex);
            auto nameIt = m_nameToId.find(name);
            if (nameIt != m_nameToId.end())
            {
                m_idToName.erase(nameIt->second);
                Remove(nameIt->second);
            }
        }

        void Clear()
        {
            std::scoped_lock lock(m_mutex);
            for (auto& [id, resource] : m_resourcesById)
            {
                m_freeHandles.push(id);
            }
            m_resourcesById.clear();
            m_nameToId.clear();
            m_idToName.clear();
        }

        const std::unordered_map<uint32_t, std::unique_ptr<T>>& GetResources() const
        {
            std::scoped_lock lock(m_mutex);
            return m_resourcesById;
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
        std::unordered_map<uint32_t, std::unique_ptr<T>> m_resourcesById;
        std::unordered_map<std::string, uint32_t> m_nameToId;
        std::unordered_map<uint32_t, std::string> m_idToName; // Reverse lookup
        std::stack<uint32_t> m_freeHandles;
        uint32_t m_nextHandle = 1;
        mutable std::mutex m_mutex;
    };
}

#endif
