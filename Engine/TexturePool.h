#ifndef TEXTURE_POOL_H
#define TEXTURE_POOL_H

#include "Graphics.h"
#include "Texture.h"

#include <unordered_map>
#include "TextureFactory.h"

namespace JLEngine
{
    class TexturePool
    {
    public:

        ~TexturePool()
        {
            for (auto& entry : m_pool)
            {
                for (auto texture : entry.second)
                {
                    Graphics::DisposeTexture(texture);
                    delete texture;
                }
            }
        }

        Texture* RequestTexture(int width, int height,
            GLenum internalFormat, 
            GLenum texType = GL_TEXTURE_2D, 
            GLenum dataType = GL_UNSIGNED_BYTE)
        {
            TextureKey key{ width, height, internalFormat, texType };

            auto it = m_pool.find(key);
            if (it != m_pool.end() && !it->second.empty())
            {
                Texture* texture = it->second.back();
                it->second.pop_back();
                return texture;
            }
           
            Texture* newTexture = TextureFactory::Create(width, height, internalFormat, texType, dataType);
            return newTexture;
        }

        void ReleaseTexture(Texture* texture)
        {
            const ImageData& data = texture->GetImageData();
            const TexParams& params = texture->GetParams();
            TextureKey key{ data.width, data.height, params.internalFormat, params.textureType };

            m_pool[key].push_back(texture);
        }

    private:

        struct TextureKey
        {
            int width;
            int height;
            GLenum format;
            GLenum type;

            bool operator==(const TextureKey& other) const
            {
                return width == other.width && height == other.height &&
                    format == other.format && type == other.type;
            }
        };

        struct KeyHash
        {
            size_t operator()(const TextureKey& key) const
            {
                return std::hash<int>()(key.width) ^
                    std::hash<int>()(key.height) ^
                    std::hash<int>()(key.format) ^
                    std::hash<int>()(key.type);
            }
        };

        // Pool mapping keys to a vector of reusable textures
        std::unordered_map<TextureKey, std::vector<Texture*>, KeyHash> m_pool;
    };
}

#endif
