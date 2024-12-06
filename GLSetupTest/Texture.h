#ifndef TEXTURE_H
#define TEXTURE_H

#include "Resource.h"
#include "Graphics.h"

namespace JLEngine
{
	class Graphics;

    class Texture : public Resource
    {
    public:
        // Constructor for file-based texture
        Texture(uint32_t handle, const std::string& name, const std::string& filename);
        Texture(const std::string& name, const std::string& filename);
        // Constructor for raw-data texture
        Texture(const std::string& name, uint32_t width, uint32_t height, void* data, int channels);
        Texture(const std::string& name, uint32_t width, uint32_t height, std::vector<unsigned char> data, int channels);

        ~Texture();

        // Initialize texture with raw data
        void InitFromData(const std::vector<unsigned char>& data, int width, int height, int channels, bool clamped, bool mipmaps);
        void InitFromData(void* data, int width, int height, int channels, int dataType, bool clamped, bool mipmaps);
        void FreeData() { m_data.clear(); }

        // Upload texture data to GPU
        void UploadToGPU(Graphics* graphics, bool freeData);

        void SetGPUID(uint32 id) { m_id = id; }
        void SetFormat(GLenum internalFormat, GLenum format, GLenum dataType);
        void SetClamped(bool clamped) { m_clamped = clamped; }
        void EnableMipmaps(bool enable) { m_mipmaps = enable; }
        
        uint32 GetFormat() const { return m_format; }
        uint32 GetInternalFormat() const { return m_internalFormat; }
        bool IsClamped() const { return m_clamped; }
        uint32_t GetGPUID() const { return m_id; }
        int GetDataType() const { return m_dataType; }
        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }
        int GetChannels() const { return m_channels; }
        const std::vector<unsigned char>& GetData() const { return m_data; }

    private:
        Graphics* m_graphics = nullptr;  // Pointer to Graphics system for texture operations

        std::string m_filename;          // For file-based textures

        uint32_t m_width = 0, m_height = 0; // Texture dimensions
        int m_channels = 0;              // Number of channels (e.g., 3 for RGB, 4 for RGBA)
        std::vector<unsigned char> m_data;  // Texture raw data (replacing `void*` for safety)

        bool m_clamped = false;          // Clamping option
        bool m_mipmaps = false;          // Mipmap option

        GLenum m_internalFormat;         // OpenGL internal format (e.g., GL_RGBA8)
        GLenum m_format;                 // OpenGL format (e.g., GL_RGBA)
        GLenum m_dataType;               // OpenGL data type (e.g., GL_UNSIGNED_BYTE)

        uint32_t m_id = 0;               // OpenGL texture ID
    };
}

#endif