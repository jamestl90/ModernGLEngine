#include "TextureManager.h"
#include "Texture.h"

namespace JLEngine
{
    // Load texture from file

    Texture* TextureManager::LoadTextureFromFile(const std::string& name, const std::string& filename, bool clamped, bool mipmaps)
    {
        return Add(name, [&]()
            {
                TextureReader reader;
                std::vector<unsigned char> data;
                int width, height, channels;

                if (!reader.ReadTexture(filename, data, width, height, channels))
                {
                    std::cerr << "Failed to load texture: " << filename << std::endl;
                    return std::unique_ptr<Texture>(nullptr);
                }

                // Create a new Texture object
                auto texture = std::make_unique<Texture>(GenerateHandle(), name, width, height, data.data(), channels);
                texture->InitFromData(data, width, height, channels, clamped, mipmaps);
                texture->UploadToGPU(m_graphics, true);
                return texture;
            });
    }

    // Load texture from raw data

    Texture* TextureManager::LoadTextureFromData(const std::string& name, uint32 width, uint32_t height, int channels, void* data, GLenum internalFormat, GLenum format, GLenum dataType, bool clamped, bool mipmaps)
    {
        return Add(name, [&]()
            {
                auto texture = std::make_unique<Texture>(GenerateHandle(), name, width, height, data, channels);
                texture->SetFormat(internalFormat, format, dataType);
                texture->SetClamped(clamped);
                texture->EnableMipmaps(mipmaps);
                texture->UploadToGPU(m_graphics, true);
                return texture;
            });
    }
}