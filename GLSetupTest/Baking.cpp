#include "Baking.h"
#include "Texture.h"
#include "ShaderProgram.h"
#include "Geometry.h"

#include <stb_image.h>
#include <iostream>
#include <memory>
#include <glad/glad.h>

namespace JLEngine
{
    Baking::Baking(AssetLoader* assetLoader) 
        : m_hdrToCubemapShader(nullptr), m_assetLoader(assetLoader)
    {
    }

    Texture* Baking::HDRtoCubemap(const std::string& name, const std::string& assetPath, int cubeMapSize)
    {
        auto fullPath = assetPath + "Core/Baking/" + name;
        if (m_hdrToCubemapShader == nullptr)
        {
            LoadHDRtoCubemapShaders(fullPath);
        }

        int width, height, channels;

        // Load equirectangular HDR texture
        float* hdrData = stbi_loadf(name.c_str(), &width, &height, &channels, 0);
        if (!hdrData)
        {
            std::cerr << "Failed to load HDR image!" << std::endl;
            return nullptr;
        }

        if (cubeMapSize == 0)
        {
            cubeMapSize = width / 4;
        }

        auto texture = m_assetLoader->CreateEmptyTexture(name);
        texture->SetSize(cubeMapSize, cubeMapSize);
        texture->SetFormat(GL_RGB16F, GL_RGB, GL_FLOAT);
        texture->SetCubemap(true);
        texture->SetClamped(true);
        texture->EnableMipmaps(true);

        // Create an empty cubemap texture
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

        // Convert equirectangular to cubemap
        GLuint captureFBO, captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, cubeMapSize, cubeMapSize);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Framebuffer not complete! Status: " << framebufferStatus << std::endl;
            return nullptr;
        }

        // Set up cube vertices and projection matrices
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] = {
            glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };
        
        Graphics* graphics = m_assetLoader->GetGraphics();
        graphics->BindShader(m_hdrToCubemapShader->GetProgramId());
        m_hdrToCubemapShader->SetUniformi("u_EquirectangularMap", 0);
        m_hdrToCubemapShader->SetUniform("u_Projection", captureProjection);

        graphics->SetActiveTexture(0);
        graphics->BindTexture(GL_TEXTURE_2D, cubemapID);

        auto vertexBuffer = Geometry::CreateSkybox(graphics);

        for (unsigned int i = 0; i < 6; ++i)
        {
            m_hdrToCubemapShader->SetUniform("u_View", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemapID, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            graphics->BindVertexArray(vertexBuffer.GetVAO());
            graphics->DrawArrayBuffer(GL_TRIANGLES, 0, 36);
            graphics->BindVertexArray(0);
        }

        graphics->DisposeVertexBuffer(vertexBuffer);
        m_hdrToCubemapShader = nullptr;

        graphics->BindFrameBuffer(0);
        stbi_image_free(hdrData);

        texture->SetGPUID(cubemapID);
        return texture;
    }

    void Baking::CleanupInternals()
    {
        m_assetLoader->GetGraphics()->DisposeShader(m_hdrToCubemapShader);
        m_hdrToCubemapShader = nullptr;
    }

    void Baking::LoadHDRtoCubemapShaders(const std::string& fullPath)
    {
        m_hdrToCubemapShader = m_assetLoader->CreateShaderFromFile(
            "EquirectangularToCubemap",
            "equirectangular_to_cubemap_vert.glsl",
            "equirectangular_to_cubemap_frag.glsl",
            fullPath
        );
    }
}