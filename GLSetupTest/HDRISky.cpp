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

        DeleteTextures();
    }

    void HDRISky::Reload(const std::string& assetPath, const HdriSkyInitParams& initParams)
    {
        DeleteTextures();

        auto shaderAssetPath = assetPath + "Core/Shaders/";
        auto textureAssetPath = assetPath + "HDRI/";
    
        TextureReader::LoadTexture(assetPath + "HDRI/" + initParams.fileName, m_hdriSkyImageData, 0);
        int cubemapSize = m_hdriSkyImageData.width / 4;

        CubemapBaker baker(assetPath, m_resourceLoader);

        m_hdriSky = baker.HDRtoCubemap(m_hdriSkyImageData, cubemapSize, 
            true, initParams.compressionThreshold, initParams.maxValue);
        m_irradianceMap = baker.GenerateIrradianceCubemap(m_hdriSky, initParams.irradianceMapSize);
        m_prefilteredMap = baker.GeneratePrefilteredEnvMap(m_hdriSky, initParams.prefilteredMapSize, initParams.prefilteredSamples);
        m_brdfLUTMap = baker.GenerateBRDFLUT(512, 1024);
        
        m_hdriSkyImageData.hdrData.clear();
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

        //TextureReader::PrintImageData(m_hdriSkyImageData);

        Graphics::API()->Enable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        CubemapBaker baker(assetPath, m_resourceLoader);

        m_hdriSky = baker.HDRtoCubemap(m_hdriSkyImageData, cubemapSize, false, initParams.compressionThreshold, initParams.maxValue);
        m_irradianceMap = baker.GenerateIrradianceCubemap(m_hdriSky, initParams.irradianceMapSize);
        m_prefilteredMap = baker.GeneratePrefilteredEnvMap(m_hdriSky, initParams.prefilteredMapSize, initParams.prefilteredSamples);
        m_brdfLUTMap = baker.GenerateBRDFLUT(512, 1024);

        //std::array<ImageData, 6> cubemapData;
        //Graphics::API()->ReadCubemap(m_hdriSky, m_hdriSkyImageData.width, m_hdriSkyImageData.height,
        //    m_hdriSkyImageData.channels, true, cubemapData, true);
        //
        //float overallMinVal = std::numeric_limits<float>::max();
        //float overallMaxVal = std::numeric_limits<float>::lowest();
        //double overallSum = 0.0;
        //size_t overallCount = 0;
        //bool dataFound = false;
        //
        // Iterate through each of the 6 cubemap faces
        //for (int face = 0; face < 6; ++face) {
        //    const auto& faceData = cubemapData[face].hdrData; // Get ref to the float vector for this face
        //
        //    if (!faceData.empty()) {
        //        dataFound = true; // Mark that we found some data
        //        float faceMinVal = faceData[0];
        //        float faceMaxVal = faceData[0];
        //        double faceSum = 0.0;
        //        size_t faceCount = 0;
        //
        //        // Iterate through float values (R, G, B components) for this face
        //        for (float val : faceData) {
        //            if (std::isfinite(val)) {
        //                faceMinVal = std::min(faceMinVal, val);
        //                faceMaxVal = std::max(faceMaxVal, val);
        //                faceSum += val;
        //                faceCount++;
        //            }
        //        }
        //
        //        if (faceCount > 0) {
        //            // Update overall statistics
        //            overallMinVal = std::min(overallMinVal, faceMinVal);
        //            overallMaxVal = std::max(overallMaxVal, faceMaxVal);
        //            overallSum += faceSum;
        //            overallCount += faceCount;
        //
        //            // Print stats for the individual face
        //            std::cout << "  Face " << face << " Stats:" << std::endl;
        //            std::cout << "    Min Value: " << faceMinVal << std::endl;
        //            std::cout << "    Max Value: " << faceMaxVal << std::endl;
        //            std::cout << "    Average Value: " << (faceSum / faceCount) << std::endl;
        //            std::cout << "    Finite Values: " << faceCount << std::endl;
        //        }
        //        else {
        //            std::cerr << "  WARNING: No finite HDR data found for Face " << face << "!" << std::endl;
        //        }
        //    }
        //    else {
        //        std::cerr << "  WARNING: HDR Data vector is empty for Face " << face << "!" << std::endl;
        //    }
        //} // End loop over faces
        //
        //// Print overall summary statistics
        //if (dataFound && overallCount > 0) {
        //    std::cout << "  --- Overall Cubemap Stats ---" << std::endl;
        //    std::cout << "    Overall Min: " << overallMinVal << std::endl;
        //    std::cout << "    Overall Max: " << overallMaxVal << std::endl;
        //    std::cout << "    Overall Average: " << (overallSum / overallCount) << std::endl;
        //    std::cout << "    Total Finite Values: " << overallCount << std::endl;
        //}
        //else if (!dataFound) {
        //    std::cerr << "  ERROR: No data found in any cubemap face vectors!" << std::endl;
        //}
        //else {
        //    std::cerr << "  WARNING: No finite values found across all cubemap faces!" << std::endl;
        //}
        //std::cout << "---------------------------------------------" << std::endl;

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

    void HDRISky::DeleteTextures()
    {
        if (m_hdriSky > 0)
            Graphics::API()->DeleteTexture(1, &m_hdriSky);
        if (m_irradianceMap > 0)
            Graphics::API()->DeleteTexture(1, &m_irradianceMap);
        if (m_prefilteredMap > 0)
            Graphics::API()->DeleteTexture(1, &m_prefilteredMap);
        if (m_brdfLUTMap > 0)
            Graphics::API()->DeleteTexture(1, &m_brdfLUTMap);
    }
}