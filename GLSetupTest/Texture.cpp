#include "Texture.h"

namespace JLEngine
{
    Texture::Texture(uint32_t handle, const std::string& name, const std::string& filename)
        : Resource(handle, name), m_filename(filename), m_clamped(false), m_mipmaps(false),
        m_internalFormat(GL_RGBA8), m_format(GL_RGBA), m_dataType(GL_UNSIGNED_BYTE), m_id(0), m_channels(0)
    {
    }

    Texture::Texture(const std::string& name, const std::string& filename)
        : Resource(name), m_filename(filename), m_clamped(false), m_mipmaps(false),
        m_internalFormat(GL_RGBA8), m_format(GL_RGBA), m_dataType(GL_UNSIGNED_BYTE), m_id(0), m_channels(0)

    {
    }

    Texture::Texture(const std::string& name)
        : Resource(name)
    {

    }

    // Constructor for raw-data texture
    Texture::Texture(const std::string& name, uint32_t width, uint32_t height, void* data, int channels)
        : Resource(name), m_width(width), m_height(height), m_channels(channels), m_clamped(false),
        m_mipmaps(false), m_internalFormat(GL_RGBA8), m_format(GL_RGBA), m_dataType(GL_UNSIGNED_BYTE), m_id(0)
    {
        if (data)
        {
            // Copy the raw data into m_data
            size_t dataSize = width * height * channels;
            m_data.assign(static_cast<unsigned char*>(data), static_cast<unsigned char*>(data) + dataSize);
        }
    }

    Texture::Texture(const std::string& name, uint32_t width, uint32_t height, std::vector<unsigned char> data, int channels)
        : Resource(name), m_width(width), m_height(height), m_channels(channels), m_clamped(false),
        m_mipmaps(false), m_internalFormat(GL_RGBA8), m_format(GL_RGBA), m_dataType(GL_UNSIGNED_BYTE), m_id(0)
    {
        if (!data.empty())
            m_data.insert(m_data.end(), m_data.begin(), m_data.end());
    }

    Texture::~Texture()
    {
        m_data.clear();
        if (m_graphics)
        {
            m_graphics->DisposeTexture(this);
        }
        if (m_id != 0)
        {
            glDeleteTextures(1, &m_id);
        }
    }

    void Texture::SetFormat(GLenum internalFormat, GLenum format, GLenum dataType)
    {
        m_internalFormat = internalFormat;
        m_format = format;
        m_dataType = dataType;
    }

    void Texture::InitFromData(const std::vector<unsigned char>& data, int width, int height, int channels, bool clamped, bool mipmaps)
    {
        m_data = data;
        m_width = width;
        m_height = height;
        m_channels = channels;
        m_clamped = clamped;
        m_mipmaps = mipmaps;

        // Determine the appropriate OpenGL format
        switch (channels)
        {
        case 1:
            m_format = GL_RED;
            m_internalFormat = GL_R8;
            break;
        case 2:
            m_format = GL_RG;
            m_internalFormat = GL_RG8;
            break;
        case 3:
            m_format = GL_RGB;
            m_internalFormat = GL_RGB8;
            break;
        case 4:
            m_format = GL_RGBA;
            m_internalFormat = GL_RGBA8;
            break;
        default:
            m_format = GL_RGBA;
            m_internalFormat = GL_RGBA8;
            std::cerr << "Unsupported channel count: " << channels << std::endl;
            break;
        }
    }

    void Texture::InitFromData(void* data, int width, int height, int channels, int dataType, bool clamped, bool mipmaps)
    {
        auto uchardata = static_cast<unsigned char*>(data);
        unsigned long long size = width * height * channels * sizeof(unsigned char);
        m_data = std::vector<unsigned char>(uchardata, uchardata + size);
        m_width = width;
        m_height = height;
        m_channels = channels;
        m_clamped = clamped;
        m_mipmaps = mipmaps;

        // Determine the appropriate OpenGL format
        switch (channels)
        {
        case 1:
            m_format = GL_RED;
            m_internalFormat = GL_R8;
            break;
        case 2:
            m_format = GL_RG;
            m_internalFormat = GL_RG8;
            break;
        case 3:
            m_format = GL_RGB;
            m_internalFormat = GL_RGB8;
            break;
        case 4:
            m_format = GL_RGBA;
            m_internalFormat = GL_RGBA8;
            break;
        default:
            m_format = GL_RGBA;
            m_internalFormat = GL_RGBA8;
            std::cerr << "Unsupported channel count: " << channels << std::endl;
            break;
        }
    }

    void Texture::UploadToGPU(Graphics* graphics, bool freeData)
    {
        m_graphics = graphics;

        m_graphics->CreateTexture(this);

        // Clear raw data after uploading to GPU to free memory
        if (freeData)
        {
            FreeData();
        }
    }
    void Texture::UploadCubemapsToGPU(Graphics* graphics, std::array<ImageData, 6>& data)
    {
        SetGPUID(m_graphics->CreateCubemap(data));
    }
}