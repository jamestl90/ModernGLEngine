#include "ShaderStorageManager.h"
#include "ShaderStorageBuffer.h"

namespace JLEngine {

    ShaderStorageManager::ShaderStorageManager(Graphics* graphics)
        : m_graphics(graphics) {}

    ShaderStorageBuffer* ShaderStorageManager::CreateSSBO(const std::string& name, size_t size) 
    {
        return Add(name, [&]() 
            {
            auto ssbo = std::make_unique<ShaderStorageBuffer>(GenerateHandle(), name, size, m_graphics);
            ssbo->Initialize(); // Custom initialization for the SSBO
            return ssbo;
            });
    }

    void ShaderStorageManager::UpdateSSBO(const std::string& name, const void* data, size_t size) 
    {
        ShaderStorageBuffer* ssbo = Get(name);
        if (ssbo) 
        {
            ssbo->UpdateData(data, size); // Assuming ShaderStorageBuffer has an UpdateData method
        }
    }

} // namespace JLEngine
