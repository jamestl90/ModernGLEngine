#include "CubemapBaker.h"
#include "Texture.h"
#include "ShaderProgram.h"
#include "Geometry.h"
#include "TextureWriter.h"
#include "TextureReader.h"

#include <stb_image.h>
#include <stb_image_write.h>
#include <iostream>
#include <memory>
#include <glad/glad.h>
#include <algorithm>

namespace JLEngine
{
    CubemapBaker::CubemapBaker(const std::string& assetPath, ResourceLoader* assetLoader)
        : m_hdrToCubemapShader(nullptr), m_assetLoader(assetLoader), m_brdfLutShader(nullptr),
        m_irradianceShader(nullptr), m_assetPath(assetPath), m_prefilteredCubemapShader(nullptr)
    {
    }

    CubemapBaker::~CubemapBaker()
    {
        CleanupInternals();
    }

    uint32_t CubemapBaker::HDRtoCubemap(ImageData& imgData, int cubeMapSize, bool genMipmaps, float compressionThresh, float maxValue)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        auto fullPath = m_assetPath + "Core/Shaders/Baking/";
        EnsureHDRtoCubemapShadersLoaded(fullPath);
        GL_CHECK_ERROR();
        GraphicsAPI* graphics = m_assetLoader->GetGraphics();
        VertexArrayObject vao;
        Geometry::CreateBox(vao);
        Graphics::CreateVertexArray(&vao);

        if (cubeMapSize == 0)
            cubeMapSize = imgData.width / 4;
        unsigned int captureFBO;
        unsigned int captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, cubeMapSize, cubeMapSize);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            throw std::runtime_error("Framebuffer incomplete.");
        }

        unsigned int hdrTexture = 0;
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, imgData.width, imgData.height, 0, GL_RGB, GL_FLOAT, imgData.hdrData.data());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        unsigned int envCubemap;
        glGenTextures(1, &envCubemap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

        for (unsigned int face = 0; face < 6; ++face)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB16F, cubeMapSize, cubeMapSize, 0, GL_RGB, GL_FLOAT, nullptr);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, genMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        graphics->BindShader(m_hdrToCubemapShader->GetProgramId());
        m_hdrToCubemapShader->SetUniformi("u_EquirectangularMap", 0);
        m_hdrToCubemapShader->SetUniform("u_Projection", captureProjection);
        m_hdrToCubemapShader->SetUniformf("u_CompressionThreshold", compressionThresh);
        m_hdrToCubemapShader->SetUniformf("u_MaxValue", maxValue);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        graphics->SetViewport(0, 0, cubeMapSize, cubeMapSize);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        for (unsigned int i = 0; i < 6; ++i)
        {
            m_hdrToCubemapShader->SetUniform("u_View", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                throw std::runtime_error("Framebuffer incomplete.");
            }

            graphics->BindVertexArray(vao.GetGPUID());
            graphics->DrawArrayBuffer(GL_TRIANGLES, 0, 36);
            graphics->BindVertexArray(0);
        }

        //if (genMipmaps)
        //{
        //    EnsureDownsampleCubemapShadersLoaded(fullPath);
        //
        //    graphics->BindShader(m_downsampleCubemapShader->GetProgramId());
        //
        //    for (unsigned int level = 1; level < mipLevels; ++level)
        //    {
        //        int mipWidth = cubeMapSize >> level;
        //        int mipHeight = cubeMapSize >> level;
        //        glViewport(0, 0, mipWidth, mipHeight);
        //
        //        for (unsigned int face = 0; face < 6; ++face)
        //        {
        //            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, envCubemap, level);
        //            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //            m_downsampleCubemapShader->SetUniformf("u_SrcMipLevel", float(level - 1));
        //            m_downsampleCubemapShader->SetUniform("u_Projection", captureProjection);
        //            m_downsampleCubemapShader->SetUniform("u_View", captureViews[face]);
        //            glActiveTexture(GL_TEXTURE0);
        //            glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        //            m_downsampleCubemapShader->SetUniformi("u_CubeMap", 0);
        //            graphics->BindVertexArray(vertexBuffer.GetVAO());
        //            graphics->DrawArrayBuffer(GL_TRIANGLES, 0, 36);
        //        }
        //    }
        //}

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        Graphics::DisposeVertexArray(&vao);        

        if (genMipmaps)
        {
            glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
            glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        }

        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);
        glDeleteTextures(1, &hdrTexture);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
        GL_CHECK_ERROR();
        return envCubemap;
    }

    uint32_t CubemapBaker::GenerateIrradianceCubemap(uint32_t hdrCubeMapID, unsigned int irradianceMapSize)
    {
        auto fullPath = m_assetPath + "Core/Shaders/Baking/";
        EnsureIrradianceShadersLoaded(fullPath);

        GLuint irradianceMap;
        glGenTextures(1, &irradianceMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

        for (unsigned int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, irradianceMapSize, irradianceMapSize, 0, GL_RGB, GL_FLOAT, nullptr);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Create a framebuffer and renderbuffer for rendering
        GLuint captureFBO, captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, irradianceMapSize, irradianceMapSize);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        GraphicsAPI* graphics = m_assetLoader->GetGraphics();
        graphics->BindShader(m_irradianceShader->GetProgramId());
        m_irradianceShader->SetUniformi("u_EnvironmentMap", 0);

        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] = 
        {
            glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
        };

        m_irradianceShader->SetUniform("u_Projection", captureProjection);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, hdrCubeMapID);

        VertexArrayObject vao;
        Geometry::CreateBox(vao);
        Graphics::CreateVertexArray(&vao);

        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        glViewport(0, 0, irradianceMapSize, irradianceMapSize);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

        for (unsigned int i = 0; i < 6; ++i)
        {
            m_irradianceShader->SetUniform("u_View", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                glDeleteFramebuffers(1, &captureFBO);
                throw std::runtime_error("Framebuffer is incomplete for Texture2D.");
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            graphics->BindVertexArray(vao.GetGPUID());
            graphics->DrawArrayBuffer(GL_TRIANGLES, 0, 36);
            graphics->BindVertexArray(0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Clean up
        glDeleteFramebuffers(1, &captureFBO);
        Graphics::DisposeVertexArray(&vao);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
        GL_CHECK_ERROR();
        return irradianceMap;
    }

    uint32_t CubemapBaker::GeneratePrefilteredEnvMap(uint32_t hdrCubeMapID, unsigned int prefEnvMapSize, int numSamples)
    {
        std::cout << "Generating Prefiltered Environment Map from HDRCubeMap..." << std::endl;

        auto fullPath = m_assetPath + "Core/Shaders/Baking/";
        EnsurePrefilteredCubemapShadersLoaded(fullPath);

        GLuint captureFBO, captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);

        GLuint prefilteredEnvMap;
        glGenTextures(1, &prefilteredEnvMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilteredEnvMap);

        for (unsigned int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, prefEnvMapSize, prefEnvMapSize, 0, GL_RGB, GL_FLOAT, nullptr);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilteredEnvMap);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        auto graphics = m_assetLoader->GetGraphics();

        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        graphics->BindShader(m_prefilteredCubemapShader->GetProgramId());
        m_prefilteredCubemapShader->SetUniformi("u_EnvironmentMap", 0);
        m_prefilteredCubemapShader->SetUniform("u_Projection", captureProjection);
        graphics->SetActiveTexture(0);
        graphics->BindTexture(GL_TEXTURE_CUBE_MAP, hdrCubeMapID);

        VertexArrayObject vao;
        Geometry::CreateBox(vao);
        Graphics::CreateVertexArray(&vao);

        unsigned int maxMips = static_cast<int>(1 + std::floor(std::log2(prefEnvMapSize)));
        for (unsigned int mip = 0; mip < maxMips; mip++)
        {
            unsigned int mipWidth = std::max(1u, prefEnvMapSize >> mip); 
            unsigned int mipHeight = std::max(1u, prefEnvMapSize >> mip);
            glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMips - 1);

            m_prefilteredCubemapShader->SetUniformf("u_Roughness", roughness);
            m_prefilteredCubemapShader->SetUniformi("u_NumSamples", numSamples);

            for (unsigned int face = 0; face < 6; face++)
            {
                m_prefilteredCubemapShader->SetUniform("u_View", captureViews[face]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, prefilteredEnvMap, mip);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) 
                {
                    throw std::runtime_error("Framebuffer incomplete after modifying attachment.");
                }

                graphics->BindVertexArray(vao.GetGPUID());
                graphics->DrawArrayBuffer(GL_TRIANGLES, 0, 36);
                graphics->BindVertexArray(0);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        Graphics::DisposeVertexArray(&vao);

        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        glUseProgram(0); 
        GL_CHECK_ERROR();
        return prefilteredEnvMap;
    }

    uint32_t CubemapBaker::GenerateBRDFLUT(int lutSize, int numSamples)
    {
        auto graphics = m_assetLoader->GetGraphics();
        auto fullPath = m_assetPath + "Core/Shaders/Baking/";
        EnsureBRDFLutShaderLoaded(fullPath);

        VertexArrayObject vao;
        JLEngine::Geometry::CreateScreenSpaceQuad(vao);
        JLEngine::Graphics::CreateVertexArray(&vao);
        auto& vbo = vao.GetVBO();
        auto& ibo = vao.GetIBO();

        GLuint captureFBO, captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        unsigned int brdfLUTTexture;
        glGenTextures(1, &brdfLUTTexture);

        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, lutSize, lutSize, 0, GL_RG, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, lutSize, lutSize);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

        graphics->SetViewport(0, 0, lutSize, lutSize);
        graphics->BindShader(m_brdfLutShader->GetProgramId());
        m_brdfLutShader->SetUniformi("u_NumSamples", numSamples);
        graphics->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        graphics->BindVertexArray(vao.GetGPUID());
        graphics->DrawElementBuffer(GL_TRIANGLES, (uint32_t)ibo.GetDataImmutable().size(), GL_UNSIGNED_INT, nullptr);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        Graphics::DisposeVertexArray(&vao);

        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);

        return brdfLUTTexture;
    }

    uint32_t CubemapBaker::CreateEmptyCubemap(int cubeMapSize)
    {
        GLuint cubemapID;
        glGenTextures(1, &cubemapID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

        for (unsigned int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB32F, cubeMapSize, cubeMapSize, 0, GL_RGB, GL_FLOAT, nullptr);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return cubemapID;
    }

    void CubemapBaker::CleanupInternals()
    {
        if (!Graphics::Alive()) return;

        m_assetLoader->DeleteShader("EquirectangularToCubemap");
        m_hdrToCubemapShader = nullptr;

        m_assetLoader->DeleteShader("IrradianceShader");
        m_irradianceShader = nullptr;

        m_assetLoader->DeleteShader("PrefilteredEnvCubemapShader");
        m_prefilteredCubemapShader = nullptr;
        
        m_assetLoader->DeleteShader("BRDFLUTShader");
        m_brdfLutShader = nullptr;
    }

    void CubemapBaker::EnsureHDRtoCubemapShadersLoaded(const std::string& fullPath)
    {
        if (m_hdrToCubemapShader == nullptr) 
        {
            m_hdrToCubemapShader = m_assetLoader->CreateShaderFromFile(
                "EquirectangularToCubemap",
                "equirectangular_to_cubemap_vert.glsl",
                "equirectangular_to_cubemap_frag.glsl",
                fullPath
            ).get();

        }
    }

    void CubemapBaker::EnsureIrradianceShadersLoaded(const std::string& fullPath)
    {
        if (m_irradianceShader == nullptr)
        {
            m_irradianceShader = m_assetLoader->CreateShaderFromFile(
                "IrradianceShader",
                "irradiance_vert.glsl",
                "irradiance_frag.glsl",
                fullPath
            ).get();
        }
    }

    void CubemapBaker::EnsurePrefilteredCubemapShadersLoaded(const std::string& fullPath)
    {
        if (m_prefilteredCubemapShader == nullptr)
        {
            m_prefilteredCubemapShader = m_assetLoader->CreateShaderFromFile(
                "PrefilteredEnvCubemapShader",
                "prefiltered_env_map_vert.glsl",
                "prefiltered_env_map_frag.glsl",
                fullPath
            ).get();
        }
    }

    void CubemapBaker::EnsureBRDFLutShaderLoaded(const std::string& fullPath)
    {
        if (m_brdfLutShader == nullptr)
        {
            m_brdfLutShader = m_assetLoader->CreateShaderFromFile(
                "BRDFLUTShader", 
                "generate_brdf_lut_vert.glsl", 
                "generate_brdf_lut_frag.glsl", 
                fullPath
            ).get();
        }
    }
}