#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include "ResourceManager.h"
#include "Graphics.h"
#include "Texture.h"
#include "TextureReader.h"

namespace JLEngine
{
	class Texture;

    class TextureManager : public ResourceManager<Texture>
    {
    public:
        TextureManager(Graphics* graphics) : m_graphics(graphics)
        {}

        // Load texture from file
        Texture* LoadTextureFromFile(const std::string& name, const std::string& filename,
            bool clamped = false, bool mipmaps = false);

        // Load texture from raw data
        Texture* LoadTextureFromData(const std::string& name, uint32 width, uint32_t height, int channels, void* data,
            GLenum internalFormat, GLenum format, GLenum dataType, bool clamped = false, bool mipmaps = false);

    private:
        Graphics* m_graphics;
    };
}

#endif