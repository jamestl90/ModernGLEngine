#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include "GraphicsResourceManager.h"
#include "Graphics.h"
#include "Texture.h"

namespace JLEngine
{
	class Texture;
	class RenderSystem;

	class TextureManager : public GraphicsResourceManager<Texture>
	{
	public:

		TextureManager(Graphics* renderSystem);

		~TextureManager();

		uint32 Add(string& name, string& path, bool fromFile = true, bool clampToEdge = false, float repeat = 1.0f);

		uint32 Add(string& fullpath, bool fromFile = true, bool clampToEdge = false, float repeat = 1.0f);
	};
}

#endif