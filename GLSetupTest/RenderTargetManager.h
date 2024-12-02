#ifndef RENDER_TARGET_MANAGER_H
#define RENDER_TARGET_MANAGER_H

#include <string>

#include "Types.h"
#include "ResourceManager.h"
#include "Buffer.h"

namespace JLEngine
{
	class RenderTarget;
	class Graphics;
	struct TextureAttribute;

	class RenderTargetManager : public ResourceManager<RenderTarget>
	{
	public:
		RenderTargetManager(Graphics* graphics) : m_graphics(graphics) {}
		~RenderTargetManager();

		RenderTarget* CreateRenderTarget(const std::string& name, int width, int height, TextureAttribute& texAttrib,
			bool depth, uint32 numSources);

		RenderTarget* CreateRenderTarget(const std::string& name, int width, int height, SmallArray<TextureAttribute>& texAttribs,
			bool depth, uint32 numSources);

	protected:
		Graphics* m_graphics;
	};
}

#endif