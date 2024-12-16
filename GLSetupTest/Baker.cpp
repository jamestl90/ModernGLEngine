#include "Baker.h"
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

namespace JLEngine
{
    Baker::Baker(const std::string& assetPath, AssetLoader* assetLoader)
        : m_hdrToCubemapShader(nullptr), m_assetLoader(assetLoader), m_irradianceShader(nullptr), m_assetPath(assetPath)
    {
    }

    Baker::~Baker()
    {
        CleanupInternals();
    }

    uint32 Baker::HDRtoCubemap(ImageData imgData, int cubeMapSize)
    {
        auto fullPath = m_assetPath + "Core/Baking/";
        EnsureHDRtoCubemapShadersLoaded(fullPath);

        if (cubeMapSize == 0)
            cubeMapSize = imgData.width / 4;

        GLuint cubemapID = CreateEmptyCubemap(cubeMapSize);
        SetupCaptureFramebuffer(cubeMapSize, cubemapID);

        RenderCubemapFromHDR(cubeMapSize, cubemapID, imgData);

        return cubemapID;
    }

    uint32 Baker::GenerateIrradianceCubemap(uint32 cubeMapId, int irradianceMapSize)
    {
        auto fullPath = m_assetPath + "Core/Baking/";
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
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, irradianceMapSize, irradianceMapSize);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            glDeleteFramebuffers(1, &captureFBO);
            throw std::runtime_error("Framebuffer is incomplete for Texture2D.");
        }

        Graphics* graphics = m_assetLoader->GetGraphics();
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
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapId);

        auto vertexBuffer = Geometry::CreateSkybox(graphics);

        glDisable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glViewport(0, 0, irradianceMapSize, irradianceMapSize);

        for (unsigned int i = 0; i < 6; ++i)
        {
            m_irradianceShader->SetUniform("u_View", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            graphics->DrawArrayBuffer(GL_TRIANGLES, 0, 36);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Clean up
        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);
        graphics->DisposeVertexBuffer(vertexBuffer);

        return irradianceMap;
    }

    void Baker::RenderCubemapFromHDR(int cubeMapSize, uint32 cubemapID, const ImageData& hdrTexture)
    {
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] = {
            glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
        };

        Graphics* graphics = m_assetLoader->GetGraphics();
        graphics->BindShader(m_hdrToCubemapShader->GetProgramId());
        m_hdrToCubemapShader->SetUniformi("u_EquirectangularMap", 0);
        m_hdrToCubemapShader->SetUniform("u_Projection", captureProjection);

        auto vertexBuffer = Geometry::CreateSkybox(graphics);
        graphics->SetViewport(0, 0, cubeMapSize, cubeMapSize);
        graphics->BindVertexArray(vertexBuffer.GetVAO());

        for (unsigned int i = 0; i < 6; ++i)
        {
            m_hdrToCubemapShader->SetUniform("u_View", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemapID, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            graphics->DrawArrayBuffer(GL_TRIANGLES, 0, 36);
        }

        graphics->BindVertexArray(0);
        graphics->DisposeVertexBuffer(vertexBuffer);
        graphics->BindFrameBuffer(0);
    }

    void Baker::SetupCaptureFramebuffer(int cubeMapSize, uint32 cubemapID)
    {
        GLuint captureFBO, captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, cubeMapSize, cubeMapSize);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Framebuffer not complete!" << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    uint32 Baker::CreateEmptyCubemap(int cubeMapSize)
    {
        GLuint cubemapID;
        glGenTextures(1, &cubemapID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

        for (unsigned int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, cubeMapSize, cubeMapSize, 0, GL_RGB, GL_FLOAT, nullptr);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return cubemapID;
    }

    void Baker::CleanupInternals()
    {
        m_assetLoader->GetGraphics()->DisposeShader(m_hdrToCubemapShader);
        m_hdrToCubemapShader = nullptr;

        m_assetLoader->GetGraphics()->DisposeShader(m_irradianceShader);
        m_irradianceShader = nullptr;
    }

    void Baker::EnsureHDRtoCubemapShadersLoaded(const std::string& fullPath)
    {
        if (m_hdrToCubemapShader == nullptr) 
        {
            m_hdrToCubemapShader = m_assetLoader->CreateShaderFromFile(
                "EquirectangularToCubemap",
                "equirectangular_to_cubemap_vert.glsl",
                "equirectangular_to_cubemap_frag.glsl",
                fullPath
            );
        }
    }

    void Baker::EnsureIrradianceShadersLoaded(const std::string& fullPath)
    {
        if (m_irradianceShader == nullptr)
        {
            m_irradianceShader = m_assetLoader->CreateShaderFromFile(
                "IrradianceShader",
                "irradiance_vert.glsl",
                "irradiance_frag.glsl",
                fullPath
            );
        }
    }

    bool Baker::SaveCubemapToFile(uint32 cubemapID, int cubeMapSize, const std::string& outputFilePath)
    {
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

        for (unsigned int i = 0; i < 6; ++i)
        {
            std::vector<float> pixels(cubeMapSize * cubeMapSize * 3);
            glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, GL_FLOAT, pixels.data());

            ImageData faceData;
            faceData.width = cubeMapSize;
            faceData.height = cubeMapSize;
            faceData.channels = 3;       
            faceData.isHDR = true;       
            faceData.hdrData = pixels;   

            std::string facePath = outputFilePath + "_face_" + std::to_string(i) + ".hdr";
            if (!TextureWriter::WriteTexture(facePath, faceData))
            {
                std::cerr << "Failed to write HDR face: " << facePath << std::endl;
                return false;
            }
        }
        return true;
    }
}