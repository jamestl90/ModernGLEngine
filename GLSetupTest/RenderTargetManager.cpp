#include "RenderTargetManager.h"
#include "RenderTarget.h"

namespace JLEngine
{
    RenderTarget* RenderTargetManager::CreateRenderTarget(const std::string& name, int width, int height, TextureAttribute& texAttrib, JLEngine::DepthType depthType, uint32 numSources)
    {
        return Add(name, [&]()
            {
                auto renderTarget = std::make_unique<RenderTarget>(GenerateHandle(), name, numSources);
                renderTarget->SetDepthType(depthType);
                renderTarget->SetWidth(width);
                renderTarget->SetHeight(height);
                renderTarget->SetTextureAttribute(0, texAttrib);
                renderTarget->UploadToGPU(m_graphics);
                return renderTarget;
            });
    }
    RenderTarget* RenderTargetManager::CreateRenderTarget(const std::string& name, int width, int height, SmallArray<TextureAttribute>& texAttribs, JLEngine::DepthType depthType, uint32 numSources)
    {
        return Add(name, [&]()
            {
                auto renderTarget = std::make_unique<RenderTarget>(GenerateHandle(), name, numSources);
                renderTarget->SetDepthType(depthType);
                renderTarget->SetWidth(width);
                renderTarget->SetHeight(height);

                for (uint32 i = 0; i < numSources; i++)
                {
                    auto& attrib = texAttribs[i];
                    renderTarget->SetTextureAttribute(i, attrib);
                }
                renderTarget->UploadToGPU(m_graphics);
                return renderTarget;
            });
    }
}