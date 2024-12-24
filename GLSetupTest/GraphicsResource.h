#ifndef GRAPHICS_RESOURCE_H
#define GRAPHICS_RESOURCE_H

#include "Resource.h"
#include <variant>
#include <vector>

namespace JLEngine
{
    class Graphics; 

    class GraphicsResource : public Resource
    {
    public:
        explicit GraphicsResource(const std::string& name, Graphics* graphics)
            : Resource(name), m_graphics(graphics) {}

        virtual ~GraphicsResource() = default;

        Graphics* GetGraphics() const { return m_graphics; }

        // Mandatory GPU-related functions
        virtual void UploadToGPU() = 0;
        virtual void UnloadFromGPU() = 0;

        void SetGPUID(uint32_t id) { m_gpuID = id; }
        void SetGPUIDs(const std::vector<uint32_t>& ids) { m_gpuID = ids; }

        uint32_t GetSingleGPUID() const
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
        Graphics* m_graphics; // Graphics context
        std::variant<uint32_t, std::vector<uint32_t>> m_gpuID;
    };
}

#endif