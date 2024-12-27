#include "HDRISky.h"
#include "Texture.h"
#include "ShaderProgram.h"
#include "GraphicsAPI.h"
#include "Geometry.h"
#include "ResourceLoader.h"
#include "TextureReader.h"

namespace JLEngine
{
    HDRISky::HDRISky(ResourceLoader* resourceLoader)
        : m_resourceLoader(resourceLoader), m_skyShader(nullptr)
    {
        
    }

    HDRISky::~HDRISky() 
    { 
        Graphics::DisposeVertexArray(&m_skyboxVAO);
        Graphics::DisposeShader(m_skyShader);
    }

    void HDRISky::Initialise(const std::string& assetPath, const HdriSkyInitParams& initParams)
    {
        auto shaderAssetPath = assetPath + "Core/Shaders/";
        auto textureAssetPath = assetPath + "HDRI/";

        m_skyShader = m_resourceLoader->CreateShaderFromFile("SkyboxShader", "enviro_cubemap_vert.glsl", "enviro_cubemap_frag.glsl", shaderAssetPath).get();
        Geometry::CreateBox(m_skyboxVAO);
        Graphics::CreateVertexArray(&m_skyboxVAO);

        TextureReader::LoadTexture(assetPath + "HDRI/" + initParams.fileName, m_hdriSkyImageData, 0);
        int cubemapSize = m_hdriSkyImageData.width / 4;

        Graphics::API()->Enable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        CubemapBaker baker(assetPath, m_resourceLoader);
        GL_CHECK_ERROR();
        m_hdriSky = baker.HDRtoCubemap(m_hdriSkyImageData, cubemapSize, true);
        m_irradianceMap = baker.GenerateIrradianceCubemap(m_hdriSky, initParams.irradianceMapSize);
        m_prefilteredMap = baker.GeneratePrefilteredEnvMap(m_hdriSky, initParams.prefilteredMapSize, initParams.prefilteredSamples);
        m_brdfLUTMap = baker.GenerateBRDFLUT(512, 1024);        
        GL_CHECK_ERROR();
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

        GLint previousDepthFunc;
        glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunc);

        Graphics::API()->SetDepthFunc(GL_LEQUAL);
        Graphics::API()->BindShader(m_skyShader->GetProgramId());
        
        m_skyShader->SetUniform("u_Projection", projMatrix);
        m_skyShader->SetUniform("u_View", glm::mat4(glm::mat3(viewMatrix)));
        Graphics::API()->SetActiveTexture(0);
        Graphics::API()->BindTexture(GL_TEXTURE_CUBE_MAP, texIds[debugTexId]);
        m_skyShader->SetUniformi("u_Skybox", 0);

        Graphics::API()->BindVertexArray(m_skyboxVAO.GetGPUID());
        Graphics::API()->DrawArrayBuffer(GL_TRIANGLES, 0, 36);

        Graphics::API()->BindVertexArray(0);
        Graphics::API()->SetDepthFunc(previousDepthFunc);
    }
}