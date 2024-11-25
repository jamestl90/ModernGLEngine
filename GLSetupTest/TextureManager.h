#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include "ResourceManager.h"
#include "Graphics.h"
#include "Texture.h"
#include "TextureReader.h"

namespace JLEngine
{
	class Texture;
	class RenderSystem;

    class TextureManager : public ResourceManager<Texture>
    {
    public:
        TextureManager(Graphics* graphics) : m_graphics(graphics)
        {}

        // Load texture from file
        std::shared_ptr<Texture> LoadTextureFromFile(const std::string& name, const std::string& filename,
            bool clamped = false, bool mipmaps = false)
        {
            return Add(name, [&]()
                {
                    // Create a texture reader to load the texture data
                    TextureReader reader;
                    std::vector<unsigned char> data;
                    int width, height, channels;

                    if (!reader.ReadTexture(filename, data, width, height, channels))
                    {
                        std::cerr << "Failed to load texture: " << filename << std::endl;
                        return std::shared_ptr<Texture>(nullptr);
                    }

                    // Create a new Texture object
                    auto texture = std::make_shared<Texture>(GenerateHandle(), name, width, height, data.data(), channels);
                    texture->InitFromData(data, width, height, channels, clamped, mipmaps);
                    texture->UploadToGPU(m_graphics, true);
                    return texture;
                });
        }

        // Load texture from raw data
        std::shared_ptr<Texture> LoadTextureFromData(const std::string& name, uint32 width, uint32_t height, int channels, void* data,
            GLenum internalFormat, GLenum format, GLenum dataType, bool clamped = false, bool mipmaps = false)
        {
            return Add(name, [&]()
                {
                    auto texture = std::make_shared<Texture>(GenerateHandle(), name, width, height, data, channels);
                    texture->SetFormat(internalFormat, format, dataType);
                    texture->SetClamped(clamped);
                    texture->EnableMipmaps(mipmaps);
                    texture->UploadToGPU(m_graphics, true);
                    return texture;
                });
        }

    private:
        Graphics* m_graphics;
    };
}

#endif