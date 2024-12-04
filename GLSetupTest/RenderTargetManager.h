#ifndef RENDER_TARGET_MANAGER_H
#define RENDER_TARGET_MANAGER_H

#include <string>

#include "Types.h"
#include "ResourceManager.h"
#include "Buffer.h"

namespace JLEngine
{
	enum class DepthType;
	class RenderTarget;
	class Graphics;
	struct TextureAttribute;

	class RenderTargetManager : public ResourceManager<RenderTarget>
	{
	public:
		RenderTargetManager(Graphics* graphics) : m_graphics(graphics) {}

		RenderTarget* CreateRenderTarget(const std::string& name, int width, int height, TextureAttribute& texAttrib,
			JLEngine::DepthType depthType, uint32 numSources);

		RenderTarget* CreateRenderTarget(const std::string& name, int width, int height, SmallArray<TextureAttribute>& texAttribs,
			JLEngine::DepthType depthType, uint32 numSources);

	protected:
		Graphics* m_graphics;
	};
}

#endif