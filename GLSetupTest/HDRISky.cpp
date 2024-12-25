#include "HDRISky.h"
#include "Texture.h"
#include "ShaderProgram.h"
#include "GraphicsAPI.h"
#include "Geometry.h"
#include "ResourceLoader.h"
#include "TextureReader.h"

namespace JLEngine
{
    HDRISky::HDRISky(ResourceLoader* assetLoader)
        : m_assetLoader(assetLoader)
    {
        
    }

    HDRISky::~HDRISky() 
    { 
        auto graphics = m_assetLoader->GetGraphics();
        graphics->DisposeVertexBuffer(m_skyboxVBO);
        graphics->DisposeShader(m_skyShader);
    }

    void HDRISky::Initialise(const std::string& assetPath, const HdriSkyInitParams& initParams)
    {
        auto shaderAssetPath = assetPath + "Core/Shaders/";
        auto textureAssetPath = assetPath + "HDRI/";
        auto graphics = m_assetLoader->GetGraphics();

        m_skyShader = m_assetLoader->CreateShaderFromFile("SkyboxShader", "enviro_cubemap_vert.glsl", "enviro_cubemap_frag.glsl", shaderAssetPath);
        m_skyboxVBO = Geometry::CreateBox(graphics);

        m_hdriSkyImageData = TextureReader::LoadTexture(assetPath + "HDRI/" + initParams.fileName, true);
        int cubemapSize = m_hdriSkyImageData.width / 4;

        graphics->Enable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        CubemapBaker baker(assetPath, m_assetLoader);
        m_hdriSky = baker.HDRtoCubemap(m_hdriSkyImageData, cubemapSize, true);
        m_irradianceMap = baker.GenerateIrradianceCubemap(m_hdriSky, initParams.irradianceMapSize);
        m_prefilteredMap = baker.GeneratePrefilteredEnvMap(m_hdriSky, initParams.prefilteredMapSize, initParams.prefilteredSamples);
        m_brdfLUTMap = baker.GenerateBRDFLUT(512, 1024);

        m_hdriSkyImageData.hdrData.clear();
    }

    void HDRISky::Render(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, int debugTexId)
    {
        if (debugTexId > 2)
        {
            std::cout << "Invalid debug tex id for HDRI SKY" << std::endl;
            return;
        }

        uint32_t texIds[3] = { m_hdriSky, m_irradianceMap, m_prefilteredMap };
        auto graphics = m_assetLoader->GetGraphics();

        GLint previousDepthFunc;
        glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunc);

        graphics->SetDepthFunc(GL_LEQUAL);
        graphics->BindShader(m_skyShader->GetProgramId());
        
        m_skyShader->SetUniform("u_Projection", projMatrix);
        m_skyShader->SetUniform("u_View", glm::mat4(glm::mat3(viewMatrix)));
        graphics->SetActiveTexture(0);
        graphics->BindTexture(GL_TEXTURE_CUBE_MAP, texIds[debugTexId]);
        m_skyShader->SetUniformi("u_Skybox", 0);

        graphics->BindVertexArray(m_skyboxVBO.GetVAO());
        graphics->DrawArrayBuffer(GL_TRIANGLES, 0, 36);

        graphics->BindVertexArray(0);
        graphics->SetDepthFunc(previousDepthFunc);
    }
}