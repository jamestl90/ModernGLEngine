#include "ImageHelpers.h"
#include "RenderTarget.h"
#include "ShaderProgram.h"
#include "GraphicsAPI.h"
#include "Graphics.h"

namespace JLEngine
{
    unsigned int ImageHelpers::m_vaoID = 0;

    ImageHelpers::ImageHelpers()
    {
        
    }

    ImageHelpers::~ImageHelpers()
    {
        if (Graphics::Alive())
            Graphics::API()->DeleteVertexArray(m_vaoID);
    }

    void ImageHelpers::Downsample(RenderTarget* input, RenderTarget* output, ShaderProgram* downsample)
    {
        if (m_vaoID == 0)
            m_vaoID = Graphics::API()->CreateVertexArray();

        auto width = output->GetWidth();
        auto height = output->GetHeight();

        Graphics::API()->BindFrameBuffer(output->GetGPUID());  
        //auto res = Graphics::API()->CheckNamedFramebufferStatus(output->GetGPUID(), GL_FRAMEBUFFER);
        //if (res != GL_FRAMEBUFFER_COMPLETE)
        //{
        //    std::cout << "ERROR" << std::endl;
        //}
        Graphics::API()->SetViewport(0, 0, width, height);
        Graphics::API()->BindShader(downsample->GetProgramId());
        Graphics::API()->BindTextureUnit(0, input->GetTexId(0));

        RenderFullscreenTriangle();
    }

    void ImageHelpers::BlurInPlaceCompute(RenderTarget* target, ShaderProgram* blurShader)
    {
        int width = target->GetWidth();
        int height = target->GetHeight();
        int workGroupCountX = (width + 15) / 16;
        int workGroupCountY = (height + 15) / 16;

        Graphics::API()->BindShader(blurShader->GetProgramId());
        blurShader->SetUniformi("u_Horizontal", 1);
        Graphics::API()->BindImageTexture(0, target->GetTexId(0), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        Graphics::API()->DispatchCompute(workGroupCountX, workGroupCountY, 1);

        blurShader->SetUniformi("u_Horizontal", 0);
        Graphics::API()->DispatchCompute(workGroupCountX, workGroupCountY, 1);
        Graphics::API()->SyncCompute();
    }

    void ImageHelpers::CopyToScreen(RenderTarget* target, int defaultWidth, int defaultHeight, ShaderProgram* prog, bool enableSRGB)
    {
        if (m_vaoID == 0)
            m_vaoID = Graphics::API()->CreateVertexArray();

        Graphics::API()->BindFrameBuffer(0);
        Graphics::API()->SetViewport(0, 0, defaultWidth, defaultHeight);
        Graphics::API()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (enableSRGB)
            Graphics::API()->Enable(GL_FRAMEBUFFER_SRGB);

        Graphics::API()->BindShader(prog->GetProgramId());
        Graphics::API()->BindTextureUnit(0, target->GetTexId(0));
        prog->SetUniformi("u_Texture", 0);

        RenderFullscreenTriangle();

        if (enableSRGB)
            Graphics::API()->Disable(GL_FRAMEBUFFER_SRGB);
    }

    void ImageHelpers::RenderFullscreenTriangle()
    {
        Graphics::API()->BindVertexArray(m_vaoID);
        Graphics::API()->DrawArrayBuffer(GL_TRIANGLES, 0, 3);
        Graphics::API()->BindVertexArray(0);
    }
}