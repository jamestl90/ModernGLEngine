#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>
#include <stdexcept>
#include <stack>

namespace JLEngine
{
    template <typename T>
    class ResourceManager
    {
    public:
        using ResourcePtr = std::shared_ptr<T>; // Shared pointer for resources
        using LoaderFunc = std::function<ResourcePtr()>; // Function to load a resource

        ResourceManager() = default;
        ~ResourceManager() = default;

        // Retrieve a resource by ID
        ResourcePtr Get(uint32_t id)
        {
            std::scoped_lock lock(m_mutex);
            auto it = m_resourcesById.find(id);
            return (it != m_resourcesById.end()) ? it->second : nullptr;
        }

        // Retrieve a resource by name
        ResourcePtr Get(const std::string& name)
        {
            std::scoped_lock lock(m_mutex);
            auto it = m_nameToId.find(name);
            return (it != m_nameToId.end()) ? Get(it->second) : nullptr;
        }

        const std::unordered_map<uint32_t, ResourcePtr> const GetResources() { return m_resourcesById; }

        // Add an existing resource
        uint32_t Add(const std::string& name, ResourcePtr resource)
        {
            std::scoped_lock lock(m_mutex);

            if (m_nameToId.find(name) != m_nameToId.end())
            {
                return m_nameToId[name]; // Return existing ID if already added
            }

            uint32_t id = GenerateHandle();
            m_resourcesById[id] = resource;
            m_nameToId[name] = id;
            m_idToName[id] = name;

            resource->SetHandle(id); // Assign unique handle to the resource

            return id;
        }

        // Load a resource using a loader function
        ResourcePtr Load(const std::string& name, const LoaderFunc& loader)
        {
            std::scoped_lock lock(m_mutex);

            auto it = m_nameToId.find(name);
            if (it != m_nameToId.end())
            {
                return Get(it->second); // Return existing resource
            }

            // Load the resource
            ResourcePtr resource = loader();
            if (!resource)
            {
                throw std::runtime_error("Failed to load resource: " + name);
            }

            Add(name, resource); // Add the resource to the manager

            return resource;
        }

        // Remove a resource by ID
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

                m_resourcesById.erase(it);
                m_freeHandles.push(id); // Reuse handle
            }
        }

        // Remove a resource by name
        void Remove(const std::string& name)
        {
            std::scoped_lock lock(m_mutex);
            auto it = m_nameToId.find(name);
            if (it != m_nameToId.end())
            {
                Remove(it->second);
            }
        }

        // Clear all resources
        void Clear()
        {
            std::scoped_lock lock(m_mutex);
            m_resourcesById.clear();
            m_nameToId.clear();
            m_idToName.clear();
            while (!m_freeHandles.empty())
            {
                m_freeHandles.pop();
            }
            m_nextHandle = 1; // Reset handle generation
        }

    private:
        // Generate a unique handle for a resource
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

        std::unordered_map<uint32_t, ResourcePtr> m_resourcesById;  // ID to Resource map
        std::unordered_map<std::string, uint32_t> m_nameToId;       // Name to ID map
        std::unordered_map<uint32_t, std::string> m_idToName;       // ID to Name map
        std::stack<uint32_t> m_freeHandles;                         // Free handles for reuse
        uint32_t m_nextHandle = 1;                                  // Handle generation counter
        std::mutex m_mutex;                                         // Mutex for thread safety
    };
}

#endif 
