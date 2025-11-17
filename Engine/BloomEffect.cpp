#include "BloomEffect.h"
#include "ResourceLoader.h"
#include "RenderTarget.h"
#include "ShaderProgram.h"
#include "ImageHelpers.h"
#include "GraphicsAPI.h"
#include "Graphics.h"

#include <algorithm>

namespace JLEngine
{
    BloomEffect::BloomEffect()
        : m_loader(nullptr)
    {
    }

    BloomEffect::~BloomEffect()
    {
        DestroyMipChain();
    }

    void BloomEffect::Initialise(ResourceLoader* loader, const std::string& assetPath, int initialWidth, int initialHeight)
    {
        m_loader = loader;
        m_assetPath = assetPath;
        auto shaderPath = m_assetPath + "Core/Shaders/";

        m_prefilterShader = m_loader->CreateShaderFromFile("BloomPrefilter", 
            "screenspacetriangle.glsl", 
            "PostProcessing/bloomprefilter_frag.glsl", 
            shaderPath).get();
        m_downsampleShader = m_loader->CreateShaderFromFile("BloomDownsample",
            "screenspacetriangle.glsl",
            "PostProcessing/bloomdownsample_frag.glsl",
            shaderPath).get();
        m_upsampleShader = m_loader->CreateShaderFromFile("BloomUpsample",
            "screenspacetriangle.glsl",
            "PostProcessing/bloomupsample_frag.glsl",
            shaderPath).get();

        if (!m_prefilterShader || !m_downsampleShader || !m_upsampleShader) 
        {
            std::cerr << "BloomEffect Error: Failed to load one or more bloom shaders." << std::endl;
            return;
        }

        m_maxIterations = 6;

        CreateMipChain(initialWidth, initialHeight);
    }

    void BloomEffect::OnResize(int newWidth, int newHeight)
    {
        CreateMipChain(newWidth, newHeight);
    }

    RenderTarget* BloomEffect::Render(RenderTarget* hdrSceneTexture, int iterations)
    {
        Graphics::API()->Disable(GL_DEPTH_TEST);
        Graphics::API()->Disable(GL_BLEND);

        RenderTarget* firstMip = m_mipChain[0].texture;
        Graphics::API()->BindFrameBuffer(firstMip->GetGPUID());
        Graphics::API()->SetViewport(0, 0, firstMip->GetWidth(), firstMip->GetHeight());
        Graphics::API()->BindShader(m_prefilterShader->GetProgramId());
        Graphics::API()->BindTextureUnit(0, hdrSceneTexture->GetTexId(0));
        m_prefilterShader->SetUniformf("u_Threshold", threshold);
        m_prefilterShader->SetUniformf("u_Knee", knee);
        ImageHelpers::RenderFullscreenTriangle();

        RenderTarget* lastDownsampleTarget = firstMip;
        int numActualIterations = std::min((int)m_mipChain.size(), iterations);

        for (int i = 1; i < numActualIterations; ++i) 
        {
            RenderTarget* currentMipTarget = m_mipChain[i].texture;
            ImageHelpers::Downsample(lastDownsampleTarget, currentMipTarget, m_downsampleShader);
            lastDownsampleTarget = currentMipTarget;
        }

        Graphics::API()->Enable(GL_BLEND);
        Graphics::API()->SetBlendFunc(GL_ONE, GL_ONE);
        Graphics::API()->SetBlendEquation(GL_FUNC_ADD);
        for (int i = numActualIterations - 2; i >= 0; --i) 
        {
            RenderTarget* lowResMip = m_mipChain[i + 1].texture;
            RenderTarget* highResMip = m_mipChain[i].texture;  

            Graphics::API()->BindFrameBuffer(highResMip->GetGPUID());
            //auto res = Graphics::API()->CheckNamedFramebufferStatus(highResMip->GetGPUID(), GL_FRAMEBUFFER);
            //if (res != GL_FRAMEBUFFER_COMPLETE)
            //{
            //    std::cout << "HERE" << std::endl;
            //}
            Graphics::API()->SetViewport(0, 0, highResMip->GetWidth(), highResMip->GetHeight());
            Graphics::API()->BindShader(m_upsampleShader->GetProgramId());

            Graphics::API()->BindTextureUnit(0, lowResMip->GetTexId(0));
            m_upsampleShader->SetUniformf("u_Scatter", scatter); 

            ImageHelpers::RenderFullscreenTriangle();
        }
        Graphics::API()->Disable(GL_BLEND);

        return m_mipChain[0].texture;
    }

    void BloomEffect::CreateMipChain(int width, int height)
    {
        DestroyMipChain();

        glm::ivec2 mipSize(width, height);

        RTParams bloomRTParams = { GL_RGBA32F, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE };

        int iterationsToAttempt = m_maxIterations;
        for (auto i = 0; i < iterationsToAttempt; i++)
        {
            if (mipSize.x == 0 || mipSize.y == 0)
                break;

            if (i == 0)
            {
                mipSize.x = std::max(1, width / 2);
                mipSize.y = std::max(1, height / 2);
            }
            else
            {
                mipSize.x = std::max(1, m_mipChain.back().size.x / 2);
                mipSize.y = std::max(1, m_mipChain.back().size.y / 2);
            }

            if (mipSize.x == 0 || mipSize.y == 0)
            {
                m_maxIterations = i;
                break;
            }

            BloomMip mip;
            mip.size = mipSize;

            std::string rtName = "BloomMip_" + std::to_string(i);
            mip.texture = m_loader->CreateRenderTarget(rtName, mipSize.x, mipSize.y, bloomRTParams, DepthType::None, 1).get();
            m_mipChain.push_back(mip);
        }
        if (m_mipChain.empty())
            std::cerr << "Error creating mips for Bloom Effect" << std::endl;
    }

    void BloomEffect::DestroyMipChain()
    {
        for (auto& mip : m_mipChain)
        {
            if (mip.texture)
            {
                m_loader->DeleteRenderTarget(mip.texture->GetName()); // Example
            }
        }
        m_mipChain.clear();
    }
}