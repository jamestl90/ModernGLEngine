#include "RenderTargetManager.h"
#include "RenderTarget.h"

namespace JLEngine
{
    RenderTargetManager::~RenderTargetManager()
    {
    }

    RenderTarget* RenderTargetManager::CreateRenderTarget(const std::string& name, int width, int height, TextureAttribute& texAttrib, bool depth, uint32 numSources)
    {
        return Add(name, [&]()
            {
                auto renderTarget = std::make_unique<RenderTarget>(GenerateHandle(), name, numSources);
                renderTarget->SetUseDepth(depth);
                renderTarget->SetWidth(width);
                renderTarget->SetHeight(height);
                renderTarget->SetTextureAttribute(0, texAttrib);
                renderTarget->UploadToGPU(m_graphics);
                return renderTarget;
            });
    }
    RenderTarget* RenderTargetManager::CreateRenderTarget(const std::string& name, int width, int height, SmallArray<TextureAttribute>& texAttribs, bool depth, uint32 numSources)
    {
        return Add(name, [&]()
            {
                auto renderTarget = std::make_unique<RenderTarget>(GenerateHandle(), name, numSources);
                renderTarget->SetUseDepth(depth);
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