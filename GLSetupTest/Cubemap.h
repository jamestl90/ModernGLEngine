#ifndef CUBEMAP_H
#define CUBEMAP_H

#include <array>

#include "ImageData.h"
#include "GraphicsResource.h"
#include "Texture.h"

namespace JLEngine
{
	class GraphicsAPI;

	class Cubemap : public GraphicsResource
	{
	public:
		Cubemap(const std::string& name, GraphicsAPI* graphics);
		~Cubemap();
		
		const std::array<ImageData, 6>& GetImageData() const { return m_cubemapFaceData; }
		std::array<ImageData, 6>& GetMutableImageData() { return m_cubemapFaceData; }

		void InitFromData(std::array<ImageData, 6>&& faceData);

		const TexParams& GetParams() const { return m_texParams; }
		void SetParams(const TexParams& params);

		static TexParams HDRCubemapParams(int channels = 3, bool mipmaps = true, bool immutable = true)
		{
			TexParams params;
			params.mipmapEnabled = mipmaps;
			params.immutable = immutable;
			params.wrapS = GL_CLAMP_TO_EDGE;
			params.wrapT = GL_CLAMP_TO_EDGE;
			params.wrapR = GL_CLAMP_TO_EDGE;
			params.minFilter = mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
			params.magFilter = GL_LINEAR;
			params.textureType = GL_TEXTURE_CUBE_MAP;
			params.internalFormat = channels == 3 ? GL_RGB16F : GL_RGBA16F;
			params.format = channels == 3 ? GL_RGB : GL_RGBA;
			params.dataType = GL_FLOAT;
		}

	protected:
		std::array<ImageData, 6> m_cubemapFaceData;
		TexParams m_texParams;
	};
}

#endif