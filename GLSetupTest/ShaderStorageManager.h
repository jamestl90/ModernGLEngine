#ifndef SHADER_STORAGE_MANAGER_H
#define SHADER_STORAGE_MANAGER_H

#include "ResourceManager.h"
#include "Graphics.h"
#include "Buffer.h" // Assuming SSBO uses a buffer-like structure
#include "ShaderStorageBuffer.h"

namespace JLEngine 
{
    class ShaderStorageBuffer;

    class ShaderStorageManager : public ResourceManager<ShaderStorageBuffer> 
    {
    public:
        ShaderStorageManager(Graphics* graphics);

        // Create a new ShaderStorageBuffer
        ShaderStorageBuffer* CreateSSBO(const std::string& name, size_t size);

        // Update SSBO with new data
        void UpdateSSBO(const std::string& name, const void* data, size_t size);

    private:
        Graphics* m_graphics; // Graphics context for OpenGL operations
    };

} // namespace JLEngine

#endif // SHADER_STORAGE_MANAGER_H
