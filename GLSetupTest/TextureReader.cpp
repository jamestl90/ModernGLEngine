#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "TextureReader.h"

namespace JLEngine
{
	TextureReader::TextureReader()
		: m_width(0), m_height(0), m_format(0), m_internalFormat(0), m_success(false)
	{
	}

	TextureReader::~TextureReader()
	{
	}

	bool TextureReader::ReadTexture( const std::string& texture )
	{
        // Use stb_image to load the image
        int width, height, channels;
        unsigned char* data = stbi_load(texture.c_str(), &width, &height, &channels, 0);

        if (data == nullptr) // Check if loading failed
        {
            m_success = false;
            return false;
        }

        // Store texture metadata
        m_width = width;
        m_height = height;
        m_internalFormat = channels; // This corresponds to the number of channels (1 for grayscale, 3 for RGB, 4 for RGBA)

        // Determine the appropriate OpenGL format based on channels
        switch (channels)
        {
        case 1: // Grayscale
            m_format = GL_RED;
            break;
        case 2: // Grayscale + Alpha
            m_format = GL_RG;
            break;
        case 3: // RGB
            m_format = GL_RGB;
            break;
        case 4: // RGBA
            m_format = GL_RGBA;
            break;
        default:
            // Handle other cases, maybe set to unknown
            m_format = 0;
            break;
        }

        // Mark as success
        m_success = true;

        // Optionally, store the image data (e.g., for later use with OpenGL)
        // For example, you could create an OpenGL texture here using the data

        // Don't forget to free the image data after usage!
        stbi_image_free(data);

        return m_success;
	}

	void TextureReader::Clear()
	{
		//ilDeleteImage(m_texture);	// free the IL texture data

		m_width = m_height = m_format = m_internalFormat = -1;
	}

	byte* TextureReader::GetData()
	{
		if (m_success)
		{
			return nullptr;
		}
		else
		{
			return nullptr;
		}
		return nullptr;
	}
}