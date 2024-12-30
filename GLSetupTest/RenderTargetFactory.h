#ifndef RENDER_TARGET_FACTORY_H
#define RENDER_TARGET_FACTORY_H

#include "ResourceManager.h"
#include "RenderTarget.h"
#include "Graphics.h"

#include <memory>
#include <string>
#include <vector>

namespace JLEngine
{
	class RenderTargetFactory
	{
	public:
		RenderTargetFactory(ResourceManager<RenderTarget>* renderTargetManager) 
			: m_renderTargetManager(renderTargetManager) {}

		std::shared_ptr<RenderTarget> CreateRenderTarget(const std::string& name, int width, int height, std::vector<TextureAttribute>& texAttribs, JLEngine::DepthType depthType, uint32_t numSources)
		{
            return m_renderTargetManager->Load(name, [&]()
                {
                    auto renderTarget = std::make_shared<RenderTarget>(name);
                    renderTarget->SetDepthType(depthType);
                    renderTarget->SetWidth(width);
                    renderTarget->SetHeight(height);
                    renderTarget->SetNumSources(numSources);

                    for (uint32_t i = 0; i < numSources; i++)
                    {
                        auto& attrib = texAttribs[i];
                        renderTarget->SetTextureAttribute(i, attrib);
                    }

                    Graphics::CreateRenderTarget(renderTarget.get());

                    return renderTarget;
                });
		}

	protected:
		ResourceManager<RenderTarget>* m_renderTargetManager;
	};
}

#endif