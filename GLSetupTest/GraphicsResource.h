#ifndef GRAPHICS_RESOURCE_H
#define GRAPHICS_RESOURCE_H

#include "Resource.h"
#include <variant>
#include <vector>

namespace JLEngine
{
    class GraphicsAPI; 

    class GraphicsResource : public Resource
    {
    public:
        explicit GraphicsResource(const std::string& name, GraphicsAPI* graphics)
            : Resource(name), m_graphics(graphics) {
            m_gpuID = (uint32_t)0;
        }

        virtual ~GraphicsResource() = default;

        GraphicsAPI* GetGraphics() const { return m_graphics; }

        void SetGPUID(uint32_t id) { m_gpuID = id; }
        void SetGPUIDs(const std::vector<uint32_t>& ids) { m_gpuID = ids; }

        uint32_t GetGPUID() const
        {
            return std::get<uint32_t>(m_gpuID);
        }

        const std::vector<uint32_t>& GetGPUIDs() const
        {
            return std::get<std::vector<uint32_t>>(m_gpuID);
        }

        bool IsSingleID() const { return std::holds_alternative<uint32_t>(m_gpuID); }
        bool IsMultiID() const { return std::holds_alternative<std::vector<uint32_t>>(m_gpuID); }

    private:
        GraphicsAPI* m_graphics; // Graphics context
        std::variant<uint32_t, std::vector<uint32_t>> m_gpuID;
    };
}

#endif