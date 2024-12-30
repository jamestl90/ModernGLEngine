#ifndef TEXTURE_FACTORY_H
#define TEXTURE_FACTORY_H

#include "ResourceManager.h"
#include "Texture.h"
#include "GraphicsAPI.h"
#include "TextureReader.h"
#include "ImageData.h"
#include "Graphics.h"

namespace JLEngine
{
    class TextureFactory
    {
    public:
        TextureFactory(ResourceManager<Texture>* textureManager, GraphicsAPI* graphics)
            : m_textureManager(textureManager), m_graphics(graphics) {}

        // Create a texture from a file
        std::shared_ptr<Texture> CreateFromFile(const std::string& name, const std::string& filePath, const TexParams& texParams, int outputChannels = 0)
        {
            return m_textureManager->Load(name, [&]() {
                ImageData imageData;
                TextureReader::LoadTexture(filePath, imageData, outputChannels);
                if (!imageData.IsValid())
                {
                    std::cerr << "Failed to load texture: " << filePath << std::endl;
                    return std::shared_ptr<Texture>(nullptr);
                }

                auto texture = std::make_shared<Texture>(name);
                texture->InitFromData(std::move(imageData));
                texture->SetParams(texParams);
                Graphics::CreateTexture(texture.get());
                return texture;
                });
        }

        std::shared_ptr<Texture> CreateFromFile(const std::string& name, const std::string& filePath)
        {
            return m_textureManager->Load(name, [&]() {
                ImageData imageData;
                TextureReader::LoadTexture(filePath, imageData, 0);
                if (!imageData.IsValid())
                {
                    std::cerr << "Failed to load texture: " << filePath << std::endl;
                    return std::shared_ptr<Texture>(nullptr);
                }

                auto texture = std::make_shared<Texture>(name);
                texture->InitFromData(std::move(imageData));
                texture->SetParams(imageData.channels == 3 ? Texture::RGBTexParams() : Texture::RGBATexParams());
                Graphics::CreateTexture(texture.get());
                return texture;
                });
        }

        // Create an empty texture
        std::shared_ptr<Texture> CreateEmpty(const std::string& name)
        {
            return m_textureManager->Load(name, [&]() {
                auto texture = std::make_shared<Texture>(name);
                return texture;
                });
        }

        // Create a texture from raw ImageData
        std::shared_ptr<Texture> CreateFromData(const std::string& name, ImageData& imageData, TexParams texParams = TexParams())
        {
            return m_textureManager->Load(name, [&]() {
                if (!imageData.IsValid())
                {
                    std::cerr << "Invalid ImageData provided for texture: " << name << std::endl;
                    return std::shared_ptr<Texture>(nullptr);
                }

                auto texture = std::make_shared<Texture>(name);
                texture->InitFromData(std::move(imageData));
                texture->SetParams(texParams);
                Graphics::CreateTexture(texture.get());
                return texture;
                });
        }

        void Delete(const std::string& name)
        {
            m_textureManager->Remove(name);
        }

    private:
        ResourceManager<Texture>* m_textureManager;
        GraphicsAPI* m_graphics;
    };
}

#endif
